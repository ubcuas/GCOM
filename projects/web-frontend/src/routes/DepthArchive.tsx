import {
    Alert,
    Box,
    Button,
    CircularProgress,
    IconButton,
    MenuItem,
    Paper,
    Stack,
    Table,
    TableBody,
    TableCell,
    TableContainer,
    TableHead,
    TableRow,
    TextField,
    Typography,
} from "@mui/material";
import ChevronLeft from "@mui/icons-material/ChevronLeft";
import ChevronRight from "@mui/icons-material/ChevronRight";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { z } from "zod";
import ImageAnnotationOverlay from "../components/ImageAnnotationOverlay";
import { getArchiveDates, getArchiveObjectFetchUrl, getArchiveRecordsForDate } from "../api/endpoints";
import type { OdlcImage } from "../schemas/odlc";
import type { OdlcImageRecord } from "../store/slices/dataSlice";
import { calculateAnnotationDistance } from "../utils/odlcImageAnnotationUtils";
import { ODLC_COLOR_LABELS, classifyOdlcColor } from "../utils/odlcColorGroup";
import type { RGB } from "../utils/odlcColorGroup";
import { fetchBinaryAsBase64 } from "../utils/base64Fetch";

const DEFAULT_COLOR_DETECTION: [number, number, number] = [128, 128, 128];
const DEFAULT_BOUNDING_BOX: Array<[number, number]> = [
    [0, 0],
    [1, 1],
];
const ARCHIVE_POLL_INTERVAL_MS = 1000;

/**
 * Backend endpoints consumed here:
 *
 * - `GET /api/vision/archive/dates/` → `string[]` (newest first, e.g. `["2026-05-11"]`).
 * - `GET /api/vision/archive/dates/<date>/` → `ArchiveRecord[]` for that date.
 *   Each record includes the bucket-relative `colorKey` (`<date>/<id>.jpg`),
 *   optional `depthKey`, and any sidecar metadata available.
 */
const ArchiveDatesSchema = z.array(z.string());

const ArchiveRecordSchema = z.object({
    id: z.string(),
    colorKey: z.string(),
    depthKey: z.string().nullable().optional(),
    receivedAt: z.union([z.string(), z.number()]).optional(),
    boundingBox: z.array(z.tuple([z.number(), z.number()])).optional(),
    confidenceLevel: z.number().optional(),
    yawDeg: z.number().nullable().optional(),
    colorDetection: z.tuple([z.number().int(), z.number().int(), z.number().int()]).optional(),
    heading: z.string().nullable().optional(),
});

const ArchiveRecordsSchema = z.array(ArchiveRecordSchema);

type ArchiveRecord = z.infer<typeof ArchiveRecordSchema>;

function yawToCompass(deg: number): string {
    const dirs = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
    const index = Math.round((((deg % 360) + 360) % 360) / 45) % 8;
    return dirs[index];
}

function formatArchiveError(prefix: string, error: unknown): string {
    if (error instanceof Error && error.message.trim().length > 0) {
        return `${prefix}: ${error.message}`;
    }
    return prefix;
}

function getRecordTimestamp(receivedAt: string | number | undefined): number {
    if (typeof receivedAt === "number" && Number.isFinite(receivedAt)) {
        return receivedAt;
    }

    if (typeof receivedAt === "string") {
        const parsed = Date.parse(receivedAt);
        if (!Number.isNaN(parsed)) {
            return parsed;
        }
    }

    return Date.now();
}

function formatArchiveMetadata(record: OdlcImageRecord): string {
    const { image, receivedAt } = record;
    const [r, g, b] = image.color_detection;
    const colorName = ODLC_COLOR_LABELS[classifyOdlcColor(image.color_detection as RGB)];
    const directionLine =
        image.yaw_deg != null
            ? `Direction: ${image.yaw_deg.toFixed(1)}° ${yawToCompass(image.yaw_deg)}`
            : "Direction: N/A";

    // Derive archive date / entry id from the composite record id ("date:uuid").
    const colonIdx = record.id.indexOf(":");
    const archiveDate = colonIdx >= 0 ? record.id.slice(0, colonIdx) : "";
    const archiveEntry = colonIdx >= 0 ? record.id.slice(colonIdx + 1) : record.id;

    const lines = [
        `Received: ${new Date(receivedAt).toISOString()}`,
        `Confidence: ${image.confidence_level}`,
        `Color: ${colorName} (RGB: ${r}, ${g}, ${b})`,
        directionLine,
        `Bounding box: ${image.bounding_box.map(([x, y]) => `(${x.toFixed(2)}, ${y.toFixed(2)})`).join(", ")}`,
        ...(archiveDate ? [`Archive date: ${archiveDate}`, `Archive entry: ${archiveEntry}`] : []),
    ];

    if (record.metadata.trim().length > 0) {
        lines.push("", record.metadata.trim());
    }

    return lines.join("\n");
}

function createArchiveImage(entry: ArchiveRecord, colorBase64: string, depthBase64: string | null): OdlcImage {
    return {
        image_data: colorBase64,
        depth_data: depthBase64,
        color_detection: entry.colorDetection ?? DEFAULT_COLOR_DETECTION,
        bounding_box: entry.boundingBox && entry.boundingBox.length >= 2 ? entry.boundingBox : DEFAULT_BOUNDING_BOX,
        confidence_level: entry.confidenceLevel ?? 0,
        yaw_deg: entry.yawDeg ?? null,
    };
}


function getArchiveItemLabel(record: OdlcImageRecord): string {
    const entryId = record.id.split(":").slice(1).join(":") || record.id;
    return `${new Date(record.receivedAt).toLocaleTimeString()} · ${entryId}`;
}

function formatSignedMeasurementMeters(value: number, digits: number): string {
    return `${value >= 0 ? "+" : ""}${value.toFixed(digits)} m`;
}

function archiveObjectFetchUrl(keyOrUrl: string): string {
    if (/^https?:\/\//i.test(keyOrUrl)) {
        return keyOrUrl;
    }
    return getArchiveObjectFetchUrl(keyOrUrl);
}

async function loadArchiveRecord(date: string, entry: ArchiveRecord): Promise<OdlcImageRecord> {
    const { colorKey, depthKey = null } = entry;
    const [imageData, depthData] = await Promise.all([
        fetchBinaryAsBase64(archiveObjectFetchUrl(colorKey)),
        depthKey ? fetchBinaryAsBase64(archiveObjectFetchUrl(depthKey)) : Promise.resolve(null),
    ]);

    return {
        id: `${date}:${entry.id}`,
        receivedAt: getRecordTimestamp(entry.receivedAt),
        image: createArchiveImage(entry, imageData, depthData),
        flagged: false,
        textInput: "",
        metadata: entry.heading ?? "",
        annotations: [],
    };
}

export default function DepthArchive() {
    const [availableDates, setAvailableDates] = useState<string[]>([]);
    const [datesLoaded, setDatesLoaded] = useState(false);
    const [datesLoading, setDatesLoading] = useState(true);
    const [datesError, setDatesError] = useState<string | null>(null);
    const [recordsLoading, setRecordsLoading] = useState(false);
    const [recordsError, setRecordsError] = useState<string | null>(null);
    const [selectedDate, setSelectedDate] = useState("");
    const [records, setRecords] = useState<OdlcImageRecord[]>([]);
    const [selectedRecordId, setSelectedRecordId] = useState<string | null>(null);
    const [highlightedAnnotationId, setHighlightedAnnotationId] = useState<string | null>(null);
    const [isPolling, setIsPolling] = useState(false);
    const [lastPolledAt, setLastPolledAt] = useState<Date | null>(null);

    // Thumbnail row: 40px img + 0.75*2 padding (12) + 1px border*2 + 0.25*8 spacing gap
    const ROW_HEIGHT_PX = 56;
    const [pageSize, setPageSize] = useState(1);
    const [page, setPage] = useState(0);

    const resizeObserverRef = useRef<ResizeObserver | null>(null);
    const listContainerCallbackRef = useCallback((el: HTMLDivElement | null) => {
        resizeObserverRef.current?.disconnect();
        resizeObserverRef.current = null;
        if (!el) return;
        const recompute = () => {
            const next = Math.max(1, Math.floor(el.clientHeight / ROW_HEIGHT_PX));
            setPageSize((prev) => (prev === next ? prev : next));
        };
        recompute();
        const ro = new ResizeObserver(recompute);
        ro.observe(el);
        resizeObserverRef.current = ro;
    }, []);

    const recordsCacheRef = useRef(new Map<string, OdlcImageRecord[]>());
    const selectedDateRef = useRef(selectedDate);
    const recordsRef = useRef(records);

    useEffect(() => {
        selectedDateRef.current = selectedDate;
    }, [selectedDate]);

    useEffect(() => {
        recordsRef.current = records;
    }, [records]);

    const totalPages = Math.max(1, Math.ceil(records.length / pageSize));
    const clampedPage = Math.min(page, totalPages - 1);
    const paginatedRecords = useMemo(
        () => records.slice(clampedPage * pageSize, (clampedPage + 1) * pageSize),
        [records, clampedPage, pageSize],
    );

    // Reset to first page when the record list or page size changes
    useEffect(() => {
        setPage(0);
    }, [records, pageSize]);

    const selectedRecord = useMemo(
        () => records.find((record) => record.id === selectedRecordId) ?? null,
        [records, selectedRecordId],
    );

    const applyRecordsUpdate = useCallback(
        (date: string, updater: (current: OdlcImageRecord[]) => OdlcImageRecord[]) => {
            const current = recordsCacheRef.current.get(date) ?? [];
            const next = updater(current);
            recordsCacheRef.current.set(date, next);

            if (selectedDateRef.current === date) {
                setRecords(next);
            }
        },
        [],
    );

    const loadDates = useCallback(async () => {
        setDatesLoading(true);
        setDatesError(null);
        setRecordsError(null);

        try {
            const parsedDates = ArchiveDatesSchema.parse(await getArchiveDates());
            recordsCacheRef.current.clear();
            setAvailableDates(parsedDates);
            setDatesLoaded(true);
            setSelectedDate((current) => (current && parsedDates.includes(current) ? current : parsedDates[0] ?? ""));
        } catch (error) {
            setAvailableDates([]);
            setDatesLoaded(false);
            setSelectedDate("");
            setRecords([]);
            setSelectedRecordId(null);
            setDatesError(formatArchiveError("Failed to load archive dates from backend", error));
        } finally {
            setDatesLoading(false);
        }
    }, []);

    useEffect(() => {
        void loadDates();
    }, [loadDates]);

    useEffect(() => {
        let cancelled = false;
        let pollInFlight = false;
        let pollInterval: ReturnType<typeof setInterval> | null = null;

        function cleanup() {
            cancelled = true;
            if (pollInterval !== null) {
                clearInterval(pollInterval);
            }
            setIsPolling(false);
            setLastPolledAt(null);
        }

        function startPolling() {
            pollInterval = setInterval(() => {
                if (cancelled || pollInFlight) {
                    return;
                }
                pollInFlight = true;
                setIsPolling(true);

                void (async () => {
                    try {
                        const date = selectedDate;
                        const entries = ArchiveRecordsSchema.parse(await getArchiveRecordsForDate(date));
                        if (cancelled) {
                            return;
                        }

                        const existingIds = new Set((recordsCacheRef.current.get(date) ?? []).map((r) => r.id));
                        const newEntries = entries.filter((entry) => !existingIds.has(`${date}:${entry.id}`));

                        if (newEntries.length > 0) {
                            const settled = await Promise.allSettled(
                                newEntries.map((entry) => loadArchiveRecord(date, entry)),
                            );
                            if (cancelled) {
                                return;
                            }

                            const loadedRecords = settled
                                .filter((r): r is PromiseFulfilledResult<OdlcImageRecord> => r.status === "fulfilled")
                                .map((r) => r.value);

                            if (loadedRecords.length > 0) {
                                applyRecordsUpdate(date, (current) => [...loadedRecords, ...current]);
                            }
                        }

                        setLastPolledAt(new Date());
                    } catch {
                        // silently skip — don't disrupt the existing view on transient failures
                    } finally {
                        pollInFlight = false;
                        if (!cancelled) {
                            setIsPolling(false);
                        }
                    }
                })();
            }, ARCHIVE_POLL_INTERVAL_MS);
        }

        if (selectedDate.length === 0) {
            setRecords([]);
            setSelectedRecordId(null);
            setRecordsLoading(false);
            return undefined;
        }

        if (recordsCacheRef.current.has(selectedDate)) {
            setRecords(recordsCacheRef.current.get(selectedDate) ?? []);
            setRecordsError(null);
            setRecordsLoading(false);
            startPolling();
            return cleanup;
        }

        setRecords([]);
        setSelectedRecordId(null);
        setRecordsError(null);
        setRecordsLoading(true);

        void (async () => {
            const entries = ArchiveRecordsSchema.parse(await getArchiveRecordsForDate(selectedDate));
            if (cancelled) {
                return;
            }

            if (entries.length === 0) {
                recordsCacheRef.current.set(selectedDate, []);
                setRecords([]);
                setRecordsLoading(false);
                startPolling();
                return;
            }

            const settled = await Promise.allSettled(entries.map((entry) => loadArchiveRecord(selectedDate, entry)));
            if (cancelled) {
                return;
            }

            const loadedRecords = settled
                .filter((result): result is PromiseFulfilledResult<OdlcImageRecord> => result.status === "fulfilled")
                .map((result) => result.value);
            const failedResults = settled.filter(
                (result): result is PromiseRejectedResult => result.status === "rejected",
            );

            recordsCacheRef.current.set(selectedDate, loadedRecords);
            setRecords(loadedRecords);
            setRecordsLoading(false);

            if (failedResults.length > 0) {
                const prefix =
                    loadedRecords.length > 0
                        ? `Loaded ${loadedRecords.length} archive images, but ${failedResults.length} failed`
                        : `Failed to load archive images for ${selectedDate}`;
                setRecordsError(formatArchiveError(prefix, failedResults[0].reason));
            }

            startPolling();
        })().catch((error) => {
            if (cancelled) {
                return;
            }

            setRecords([]);
            setSelectedRecordId(null);
            setRecordsLoading(false);
            setRecordsError(formatArchiveError(`Failed to load archive images for ${selectedDate}`, error));
        });

        return cleanup;
    }, [selectedDate, applyRecordsUpdate]);

    useEffect(() => {
        if (records.length === 0) {
            setSelectedRecordId(null);
            return;
        }

        if (selectedRecordId === null || !records.some((record) => record.id === selectedRecordId)) {
            setSelectedRecordId(records[0].id);
        }
    }, [records, selectedRecordId]);

    useEffect(() => {
        setHighlightedAnnotationId(null);
    }, [selectedRecord?.id]);

    useEffect(() => {
        if (
            highlightedAnnotationId !== null &&
            selectedRecord != null &&
            !selectedRecord.annotations.some((annotation) => annotation.id === highlightedAnnotationId)
        ) {
            setHighlightedAnnotationId(null);
        }
    }, [highlightedAnnotationId, selectedRecord]);

    const handleMeasurementRowClick = useCallback((annotationId: string) => {
        setHighlightedAnnotationId((current) => (current === annotationId ? null : annotationId));
    }, []);

    // Deltas come from the deprojected camera-frame endpoints (meters) so that
    // dX/dY share units with dZ and the Euclidean distance. They're null until
    // the async deprojection completes (or stay null if no depth was sampled).
    const measurementRows = useMemo(() => {
        if (!selectedRecord) return [];

        return selectedRecord.annotations.map((annotation) => {
            const haveCameraPoints = annotation.p1_3d != null && annotation.p2_3d != null;
            return {
                annotation,
                deltaX: haveCameraPoints ? annotation.p2_3d![0] - annotation.p1_3d![0] : null,
                deltaY: haveCameraPoints ? annotation.p2_3d![1] - annotation.p1_3d![1] : null,
                deltaZ: haveCameraPoints ? annotation.p2_3d![2] - annotation.p1_3d![2] : null,
            };
        });
    }, [selectedRecord]);

    const handleImageAnnotation = useCallback(
        async (args: {
            id: string;
            annotationId: string;
            p1: { x: number; y: number };
            p2: { x: number; y: number };
        }) => {
            const date = selectedDateRef.current;
            if (!date) {
                return;
            }

            applyRecordsUpdate(date, (current) =>
                current.map((record) =>
                    record.id === args.id
                        ? {
                              ...record,
                              annotations: [...record.annotations, { id: args.annotationId, p1: args.p1, p2: args.p2 }],
                          }
                        : record,
                ),
            );

            const currentRecord = recordsRef.current.find((record) => record.id === args.id);
            const result = await calculateAnnotationDistance(args.p1, args.p2, currentRecord?.image.depth_data ?? null);

            if (result !== null) {
                applyRecordsUpdate(date, (current) =>
                    current.map((record) =>
                        record.id === args.id
                            ? {
                                  ...record,
                                  annotations: record.annotations.map((annotation) =>
                                      annotation.id === args.annotationId
                                          ? {
                                                ...annotation,
                                                distance: result.distance,
                                                p1_3d: result.p1_3d,
                                                p2_3d: result.p2_3d,
                                            }
                                          : annotation,
                                  ),
                              }
                            : record,
                    ),
                );
            }
        },
        [applyRecordsUpdate],
    );

    const handleUndo = useCallback(
        (id: string) => {
            const date = selectedDateRef.current;
            if (!date) {
                return;
            }

            applyRecordsUpdate(date, (current) =>
                current.map((record) =>
                    record.id === id ? { ...record, annotations: record.annotations.slice(0, -1) } : record,
                ),
            );
        },
        [applyRecordsUpdate],
    );

    const handleDeleteAnnotation = useCallback(
        (args: { id: string; annotationId: string }) => {
            const date = selectedDateRef.current;
            if (!date) {
                return;
            }

            applyRecordsUpdate(date, (current) =>
                current.map((record) =>
                    record.id === args.id
                        ? {
                              ...record,
                              annotations: record.annotations.filter(
                                  (annotation) => annotation.id !== args.annotationId,
                              ),
                          }
                        : record,
                ),
            );
        },
        [applyRecordsUpdate],
    );

    return (
        <Box sx={{ p: 3, width: "100%", display: "flex", flexDirection: "column", height: "100%" }}>
            <Stack
                direction={{ xs: "column", md: "row" }}
                alignItems={{ xs: "stretch", md: "center" }}
                justifyContent="space-between"
                spacing={2}
                mb={2}
            >
                <Box>
                    <Typography variant="h5" fontWeight="bold">
                        Depth Archive
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                        Pick an archive date to load RGB images and their depth maps from S3.
                    </Typography>
                </Box>

                <Stack direction={{ xs: "column", sm: "row" }} spacing={1}>
                    <TextField
                        select
                        size="small"
                        label="Archive date"
                        value={selectedDate}
                        onChange={(event) => setSelectedDate(event.target.value)}
                        disabled={datesLoading || availableDates.length === 0}
                        sx={{ minWidth: 220 }}
                    >
                        {availableDates.length > 0 ? (
                            availableDates.map((date) => (
                                <MenuItem key={date} value={date}>
                                    {date}
                                </MenuItem>
                            ))
                        ) : (
                            <MenuItem value="" disabled>
                                No dates available
                            </MenuItem>
                        )}
                    </TextField>
                    <Button variant="outlined" onClick={() => void loadDates()} disabled={datesLoading}>
                        {datesLoading ? "Loading..." : "Reload dates"}
                    </Button>
                </Stack>
            </Stack>

            <Stack spacing={1.5} sx={{ mb: 2 }}>
                {datesError && <Alert severity="warning">{datesError}</Alert>}
                {recordsError && <Alert severity="warning">{recordsError}</Alert>}
                {!datesError && !datesLoading && datesLoaded && availableDates.length === 0 && (
                    <Alert severity="info">The archive bucket does not contain any dated captures yet.</Alert>
                )}
            </Stack>

            <Box sx={{ display: "flex", gap: 2, flex: 1, minHeight: 0 }}>
                <Paper
                    sx={{
                        width: 280,
                        flexShrink: 0,
                        p: 1.5,
                        display: "flex",
                        flexDirection: "column",
                        minHeight: 0,
                    }}
                >
                    <Typography variant="subtitle2" color="text.secondary" gutterBottom>
                        Archive Images
                    </Typography>
                    <Box sx={{ mb: 1.5 }}>
                        <Typography variant="caption" color="text.secondary" display="block">
                            {selectedDate
                                ? `${records.length} loaded for ${selectedDate}`
                                : "Select a date to load images"}
                        </Typography>
                        {selectedDate && !recordsLoading && (isPolling || lastPolledAt !== null) && (
                            <Stack direction="row" alignItems="center" spacing={0.5} sx={{ mt: 0.5 }}>
                                {isPolling && <CircularProgress size={10} />}
                                <Typography variant="caption" color="text.secondary">
                                    {isPolling
                                        ? "Checking for new images..."
                                        : `Updated ${lastPolledAt!.toLocaleTimeString()}`}
                                </Typography>
                            </Stack>
                        )}
                    </Box>

                    <Box ref={listContainerCallbackRef} sx={{ flex: 1, minHeight: 0, overflow: "hidden" }}>
                        {recordsLoading ? (
                            <Stack spacing={1} alignItems="center" justifyContent="center" sx={{ minHeight: 160 }}>
                                <CircularProgress size={24} />
                                <Typography variant="body2" color="text.secondary">
                                    Loading archive images...
                                </Typography>
                            </Stack>
                        ) : records.length > 0 ? (
                            <Stack spacing={0.25}>
                                {paginatedRecords.map((record) => (
                                    <Box
                                        key={record.id}
                                        onClick={() => setSelectedRecordId(record.id)}
                                        sx={{
                                            display: "flex",
                                            alignItems: "center",
                                            gap: 1,
                                            p: 0.75,
                                            borderRadius: 1,
                                            cursor: "pointer",
                                            border: "1px solid",
                                            borderColor:
                                                selectedRecordId === record.id ? "primary.main" : "transparent",
                                            bgcolor: selectedRecordId === record.id ? "action.selected" : "transparent",
                                            "&:hover": { bgcolor: "action.hover" },
                                        }}
                                    >
                                        <Box
                                            component="img"
                                            src={`data:image/jpeg;base64,${record.image.image_data}`}
                                            alt=""
                                            sx={{
                                                width: 40,
                                                height: 40,
                                                borderRadius: 0.5,
                                                objectFit: "cover",
                                                flexShrink: 0,
                                            }}
                                        />
                                        <Typography variant="caption" noWrap sx={{ flex: 1 }}>
                                            {getArchiveItemLabel(record)}
                                        </Typography>
                                    </Box>
                                ))}
                            </Stack>
                        ) : (
                            <Stack spacing={1} justifyContent="center" sx={{ minHeight: 160 }}>
                                <Typography variant="body2" color="text.secondary">
                                    {selectedDate
                                        ? "No archive images are available for this date yet."
                                        : "Select a date to load archive images."}
                                </Typography>
                            </Stack>
                        )}
                    </Box>

                    {records.length > pageSize && (
                        <Box
                            sx={{
                                display: "flex",
                                alignItems: "center",
                                justifyContent: "space-between",
                                mt: 2,
                                pt: 1,
                                borderTop: 1,
                                borderColor: "divider",
                            }}
                        >
                            <IconButton
                                size="small"
                                disabled={clampedPage === 0}
                                onClick={() => setPage((p) => Math.max(0, p - 1))}
                                aria-label="Previous page"
                            >
                                <ChevronLeft />
                            </IconButton>
                            <Typography variant="caption" color="text.secondary">
                                {clampedPage + 1} / {totalPages}
                            </Typography>
                            <IconButton
                                size="small"
                                disabled={clampedPage >= totalPages - 1}
                                onClick={() => setPage((p) => Math.min(totalPages - 1, p + 1))}
                                aria-label="Next page"
                            >
                                <ChevronRight />
                            </IconButton>
                        </Box>
                    )}
                </Paper>

                <Paper sx={{ flex: 1, p: 2, display: "flex", flexDirection: "column", minWidth: 0 }}>
                    <Typography variant="caption" color="text.secondary" sx={{ mb: 1 }}>
                        Click &amp; hold to draw a line between 2 points · Ctrl+Z to undo · Double-click a line to
                        delete
                    </Typography>

                    <Box
                        sx={{
                            flex: 1,
                            minHeight: 200,
                            bgcolor: "background.default",
                            borderRadius: 1,
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                            overflow: "hidden",
                            position: "relative",
                        }}
                    >
                        {selectedRecord ? (
                            <>
                                <ImageAnnotationOverlay
                                    imageSrc={`data:image/jpeg;base64,${selectedRecord.image.image_data}`}
                                    annotations={selectedRecord.annotations}
                                    recordId={selectedRecord.id}
                                    highlightedAnnotationId={highlightedAnnotationId}
                                    boundingBox={selectedRecord.image.bounding_box}
                                    depthData={selectedRecord.image.depth_data}
                                    onAddAnnotation={handleImageAnnotation}
                                    onUndo={handleUndo}
                                    onDeleteAnnotation={handleDeleteAnnotation}
                                />
                                {(selectedRecord.metadata.trim() || selectedRecord.image.yaw_deg != null) && (
                                    <Box
                                        sx={{
                                            position: "absolute",
                                            top: 8,
                                            left: 8,
                                            bgcolor: "rgba(0,0,0,0.6)",
                                            color: "#fff",
                                            borderRadius: 1,
                                            px: 1,
                                            py: 0.25,
                                            display: "flex",
                                            alignItems: "center",
                                            gap: 0.5,
                                            pointerEvents: "none",
                                        }}
                                    >
                                        {selectedRecord.image.yaw_deg != null && (
                                            <Box
                                                component="span"
                                                sx={{
                                                    display: "inline-block",
                                                    transform: `rotate(${selectedRecord.image.yaw_deg}deg)`,
                                                    fontSize: "1rem",
                                                    lineHeight: 1,
                                                }}
                                            >
                                                ↑
                                            </Box>
                                        )}
                                        <Typography variant="caption" sx={{ color: "#fff", fontFamily: "monospace" }}>
                                            {selectedRecord.metadata.trim() ||
                                                `${selectedRecord.image.yaw_deg!.toFixed(1)}° ${yawToCompass(selectedRecord.image.yaw_deg!)}`}
                                        </Typography>
                                    </Box>
                                )}
                            </>
                        ) : recordsLoading ? (
                            <CircularProgress size={28} />
                        ) : (
                            <Typography color="text.secondary">Select an archive image</Typography>
                        )}
                    </Box>

                    {selectedRecord && (
                        <Stack spacing={1.5} sx={{ mt: 2 }}>
                            <Box>
                                <Typography variant="caption" color="text.secondary" display="block" gutterBottom>
                                    Measurements
                                </Typography>
                                <TableContainer component={Paper} variant="outlined" sx={{ overflowX: "auto" }}>
                                    <Table size="small">
                                        <TableHead>
                                            <TableRow>
                                                <TableCell width="1%">#</TableCell>
                                                <TableCell>Distance</TableCell>
                                                <TableCell>dX</TableCell>
                                                <TableCell>dY</TableCell>
                                                <TableCell>dZ</TableCell>
                                            </TableRow>
                                        </TableHead>
                                        <TableBody>
                                            {measurementRows.length > 0 ? (
                                                measurementRows.map(({ annotation, deltaX, deltaY, deltaZ }, index) => {
                                                    const isSelected = highlightedAnnotationId === annotation.id;
                                                    return (
                                                        <TableRow
                                                            key={annotation.id}
                                                            hover
                                                            selected={isSelected}
                                                            onClick={() => handleMeasurementRowClick(annotation.id)}
                                                            sx={{
                                                                cursor: "pointer",
                                                                "&:last-child td": { borderBottom: 0 },
                                                            }}
                                                        >
                                                            <TableCell>{index + 1}</TableCell>
                                                            <TableCell>
                                                                {annotation.distance != null
                                                                    ? `${annotation.distance.toFixed(2)} m`
                                                                    : "..."}
                                                            </TableCell>
                                                            <TableCell>
                                                                {deltaX != null
                                                                    ? formatSignedMeasurementMeters(deltaX, 2)
                                                                    : "..."}
                                                            </TableCell>
                                                            <TableCell>
                                                                {deltaY != null
                                                                    ? formatSignedMeasurementMeters(deltaY, 2)
                                                                    : "..."}
                                                            </TableCell>
                                                            <TableCell>
                                                                {deltaZ != null
                                                                    ? formatSignedMeasurementMeters(deltaZ, 2)
                                                                    : "..."}
                                                            </TableCell>
                                                        </TableRow>
                                                    );
                                                })
                                            ) : (
                                                <TableRow>
                                                    <TableCell colSpan={5} sx={{ color: "text.secondary" }}>
                                                        No measurements yet
                                                    </TableCell>
                                                </TableRow>
                                            )}
                                        </TableBody>
                                    </Table>
                                </TableContainer>
                            </Box>
                            <Box>
                                <Typography variant="caption" color="text.secondary" display="block" gutterBottom>
                                    Metadata
                                </Typography>
                                <Box
                                    sx={{
                                        p: 1.5,
                                        borderRadius: 1,
                                        bgcolor: "action.hover",
                                        fontFamily: "monospace",
                                        fontSize: "0.75rem",
                                        whiteSpace: "pre-wrap",
                                        wordBreak: "break-all",
                                    }}
                                >
                                    {formatArchiveMetadata(selectedRecord)}
                                </Box>
                            </Box>
                        </Stack>
                    )}
                </Paper>
            </Box>
        </Box>
    );
}
