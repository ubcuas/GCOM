import {
    Box,
    Button,
    FormControlLabel,
    IconButton,
    MenuItem,
    Paper,
    Stack,
    Switch,
    Table,
    TableBody,
    TableCell,
    TableContainer,
    TableHead,
    TableRow,
    TextField,
    ToggleButton,
    ToggleButtonGroup,
    Typography,
} from "@mui/material";
import ChevronLeft from "@mui/icons-material/ChevronLeft";
import ChevronRight from "@mui/icons-material/ChevronRight";
import Flag from "@mui/icons-material/Flag";
import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { useAppDispatch, useAppSelector } from "../store/store";
import {
    selectOdlcImageRecords,
    selectSelectedOdlcImageId,
    selectSelectedOdlcImageRecord,
    setSelectedOdlcImage,
    updateOdlcImageFlag,
    updateOdlcImageTextInput,
    addOdlcImageAnnotation,
    undoLastOdlcImageAnnotation,
    deleteOdlcImageAnnotation,
    setOdlcImageAnnotationMeasurements,
} from "../store/slices/dataSlice";
import ImageAnnotationOverlay from "../components/ImageAnnotationOverlay";
import SessionIdDisplay from "../components/SessionIdDisplay";
import {
    syncOdlcImageToBackend,
    appendOdlcImageFromAircraft,
    pollOdlcSession,
} from "../store/thunks/odlcSessionThunks";
import type { OdlcImageRecord } from "../store/slices/dataSlice";
import { classifyOdlcColor, ODLC_COLORS, ODLC_COLOR_LABELS } from "../utils/odlcColorGroup";
import type { OdlcColor, RGB } from "../utils/odlcColorGroup";
import type { OdlcImage } from "../schemas/odlc";
import { calculateAnnotationDistance } from "../utils/odlcImageAnnotationUtils";

type SortBy = "recency" | "confidence";
type ColorFilter = OdlcColor | "all";

/** Build a colored square image as base64 JPEG for testing without a stream. */
function createDummyImageBase64(width: number, height: number, r: number, g: number, b: number): string {
    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext("2d");
    if (!ctx) return "";
    ctx.fillStyle = `rgb(${r},${g},${b})`;
    ctx.fillRect(0, 0, width, height);
    const dataUrl = canvas.toDataURL("image/jpeg", 0.9);
    return dataUrl.split(",")[1] ?? "";
}

/** Random int in [min, max] inclusive. */
function randomInt(min: number, max: number): number {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

/** Random float in [min, max). */
function randomIn(min: number, max: number): number {
    return Math.random() * (max - min) + min;
}

/** Convert yaw degrees (clockwise from north) to cardinal/intercardinal label. */
function yawToCompass(deg: number): string {
    const dirs = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
    const index = Math.round((((deg % 360) + 360) % 360) / 45) % 8;
    return dirs[index];
}

function createDummyOdlcImage(color: [number, number, number], confidenceLevel: number): OdlcImage {
    const width = randomInt(40, 120);
    const height = randomInt(40, 120);
    const x1 = randomIn(0.05, 0.85);
    const y1 = randomIn(0.05, 0.85);
    const x2 = randomIn(0.15, 0.95);
    const y2 = randomIn(0.15, 0.95);
    return {
        image_data: createDummyImageBase64(width, height, ...color),
        depth_data: null,
        color_detection: color,
        bounding_box: [
            [x1, y1],
            [x2, y2],
        ],
        confidence_level: confidenceLevel,
        yaw_deg: Math.round(randomIn(0, 360)),
    };
}

function filterAndSortRecords(
    records: OdlcImageRecord[],
    flaggedOnly: boolean,
    sortBy: SortBy,
    colorFilter: ColorFilter,
): OdlcImageRecord[] {
    let list = flaggedOnly ? records.filter((r) => r.flagged) : [...records];
    if (colorFilter !== "all") {
        list = list.filter((r) => classifyOdlcColor(r.image.color_detection as RGB) === colorFilter);
    }
    if (sortBy === "recency") {
        list.sort((a, b) => b.receivedAt - a.receivedAt);
    } else {
        list.sort((a, b) => b.image.confidence_level - a.image.confidence_level);
    }
    return list;
}

/** Pretty-print image payload for read-only metadata display (not for export). */
function formatImageMetadata(record: OdlcImageRecord): string {
    const { image, receivedAt, metadata } = record;
    const [r, g, b] = image.color_detection;
    const colorName = ODLC_COLOR_LABELS[classifyOdlcColor(image.color_detection as RGB)];
    const directionLine = metadata.trim()
        ? metadata.trim()
        : image.yaw_deg != null
        ? `Direction: ${image.yaw_deg.toFixed(1)}°`
        : `Direction: N/A`;
    const lines = [
        `Received: ${new Date(receivedAt).toISOString()}`,
        `Confidence: ${image.confidence_level}`,
        `Color: ${colorName} (RGB: ${r}, ${g}, ${b})`,
        directionLine,
        `Bounding box: ${image.bounding_box.map(([x, y]) => `(${x.toFixed(2)}, ${y.toFixed(2)})`).join(", ")}`,
    ];
    return lines.join("\n");
}

function formatSignedMeasurementMeters(value: number, digits: number): string {
    return `${value >= 0 ? "+" : ""}${value.toFixed(digits)} m`;
}

export default function OdlcImages() {
    const records = useAppSelector(selectOdlcImageRecords);
    const selectedId = useAppSelector(selectSelectedOdlcImageId);
    const selectedRecord = useAppSelector(selectSelectedOdlcImageRecord);

    const [flaggedOnly, setFlaggedOnly] = useState(false);
    const [sortBy, setSortBy] = useState<SortBy>("recency");
    const [colorFilter, setColorFilter] = useState<ColorFilter>("all");
    const [autoRefresh, setAutoRefresh] = useState(false);
    const [highlightedAnnotationId, setHighlightedAnnotationId] = useState<string | null>(null);

    const dispatch = useAppDispatch();
    const handleSelect = useCallback((id: string) => dispatch(setSelectedOdlcImage(id)), [dispatch]);

    // Tracks the textInput value at the moment the field gained focus so onBlur
    // can skip syncing no-op edits (tab-through with no changes).
    const textInputFocusValueRef = useRef<string | null>(null);

    // Thumbnail row: 40px img + 0.75*2 padding (12) + 1px border*2 + 0.25*8 spacing gap
    const ROW_HEIGHT_PX = 56;
    const [pageSize, setPageSize] = useState(1);
    const [page, setPage] = useState(0);

    // Callback ref: fires on attach/detach so we measure correctly even when
    // the list container isn't in the DOM on first render (e.g. empty state
    // then session restore populates records and mounts the main layout).
    const pollIntervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

    useEffect(() => {
        if (autoRefresh) {
            pollIntervalRef.current = setInterval(() => {
                dispatch(pollOdlcSession());
            }, 1000);
        } else {
            if (pollIntervalRef.current !== null) {
                clearInterval(pollIntervalRef.current);
                pollIntervalRef.current = null;
            }
        }
        return () => {
            if (pollIntervalRef.current !== null) {
                clearInterval(pollIntervalRef.current);
                pollIntervalRef.current = null;
            }
        };
    }, [autoRefresh, dispatch]);

    const resizeObserverRef = useRef<ResizeObserver | null>(null);
    const listContainerCallbackRef = useCallback((el: HTMLDivElement | null) => {
        resizeObserverRef.current?.disconnect();
        resizeObserverRef.current = null;
        if (!el) return;
        const recompute = () => {
            const available = el.clientHeight;
            const next = Math.max(1, Math.floor(available / ROW_HEIGHT_PX));
            setPageSize((prev) => (prev === next ? prev : next));
        };
        recompute();
        const ro = new ResizeObserver(recompute);
        ro.observe(el);
        resizeObserverRef.current = ro;
    }, []);

    const filteredSorted = useMemo(
        () => filterAndSortRecords(records, flaggedOnly, sortBy, colorFilter),
        [records, flaggedOnly, sortBy, colorFilter],
    );

    const totalPages = Math.max(1, Math.ceil(filteredSorted.length / pageSize));
    const clampedPage = Math.min(page, totalPages - 1);
    const paginatedRecords = useMemo(
        () => filteredSorted.slice(clampedPage * pageSize, (clampedPage + 1) * pageSize),
        [filteredSorted, clampedPage, pageSize],
    );

    // Reset to first page when filters change. Do not reset when pageSize changes:
    // pageSize is driven by ResizeObserver and can flicker across row-count thresholds
    // (scrollbar, flex layout), which was sending users back to page 1 while paging.
    useEffect(() => {
        setPage(0);
    }, [flaggedOnly, sortBy, colorFilter]);

    /** Flagged records in current filter/sort order (export includes only these). */
    const flaggedForExport = useMemo(() => filteredSorted.filter((r) => r.flagged), [filteredSorted]);

    const handleExport = useCallback(() => {
        if (flaggedForExport.length === 0) return;
        const lines = flaggedForExport.map(
            (r) => `[${r.id}] ${new Date(r.receivedAt).toISOString()}\n${r.textInput.trim() || "(no text)"}\n`,
        );
        const blob = new Blob([lines.join("\n")], { type: "text/plain;charset=utf-8" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = `odlc-export-${new Date().toISOString().slice(0, 10)}.txt`;
        a.click();
        URL.revokeObjectURL(url);
    }, [flaggedForExport]);

    useEffect(() => {
        if (records.length > 0 && selectedId === null) {
            dispatch(setSelectedOdlcImage(records[0].id));
        }
    }, [records, selectedId, dispatch]);

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

    const loadSampleImages = useCallback(() => {
        const samples: OdlcImage[] = [
            createDummyOdlcImage([255, 0, 0], 92),
            createDummyOdlcImage([255, 0, 0], 88),
            createDummyOdlcImage([255, 200, 0], 85),
            createDummyOdlcImage([255, 200, 0], 85),
            createDummyOdlcImage([255, 200, 0], 85),
            createDummyOdlcImage([255, 200, 0], 78),
            createDummyOdlcImage([0, 180, 0], 95),
            createDummyOdlcImage([0, 180, 0], 70),
            createDummyOdlcImage([0, 180, 0], 60),
            createDummyOdlcImage([0, 180, 0], 55),
        ];
        samples.forEach((img) => dispatch(appendOdlcImageFromAircraft(img)));
    }, [dispatch]);

    const handleImageAnnotation = async (args: {
        id: string;
        annotationId: string;
        p1: { x: number; y: number };
        p2: { x: number; y: number };
    }) => {
        dispatch(addOdlcImageAnnotation(args));

        const res = await calculateAnnotationDistance(args.p1, args.p2, selectedRecord?.image.depth_data ?? null);
        if (res !== null) {
            dispatch(
                setOdlcImageAnnotationMeasurements({
                    id: args.id,
                    annotationId: args.annotationId,
                    distance: res.distance,
                    p1_3d: res.p1_3d,
                    p2_3d: res.p2_3d,
                }),
            );
        }
    };

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

    return (
        <Box sx={{ p: 3, width: "100%", display: "flex", flexDirection: "column", height: "100%" }}>
            <Stack direction="row" alignItems="center" justifyContent="space-between" mb={2}>
                <Typography variant="h5" fontWeight="bold">
                    ODLC Images
                </Typography>
                <SessionIdDisplay />
            </Stack>

            <Box sx={{ display: "flex", gap: 2, flex: 1, minHeight: 0 }}>
                {/* Left sidebar */}
                <Paper
                    sx={{
                        width: 280,
                        flexShrink: 0,
                        p: 1.5,
                        display: "flex",
                        flexDirection: "column",
                        minHeight: 0,
                        overflow: "hidden",
                    }}
                >
                    <Typography variant="subtitle2" color="text.secondary" gutterBottom>
                        Filters
                    </Typography>
                    <Stack spacing={1} sx={{ mb: 2 }}>
                        <FormControlLabel
                            control={
                                <Switch size="small" checked={autoRefresh} onChange={(_, v) => setAutoRefresh(v)} />
                            }
                            label={<Typography variant="body2">Auto-refresh</Typography>}
                        />
                        <FormControlLabel
                            control={
                                <Switch size="small" checked={flaggedOnly} onChange={(_, v) => setFlaggedOnly(v)} />
                            }
                            label={<Typography variant="body2">Flagged only</Typography>}
                        />
                        <Box>
                            <Typography variant="caption" color="text.secondary" display="block">
                                Sort
                            </Typography>
                            <ToggleButtonGroup
                                size="small"
                                value={sortBy}
                                exclusive
                                onChange={(_, v) => v != null && setSortBy(v)}
                                fullWidth
                            >
                                <ToggleButton value="recency">Recency</ToggleButton>
                                <ToggleButton value="confidence">Confidence</ToggleButton>
                            </ToggleButtonGroup>
                        </Box>
                        <TextField
                            select
                            size="small"
                            label="Color"
                            value={colorFilter}
                            onChange={(e) => setColorFilter(e.target.value as ColorFilter)}
                            fullWidth
                        >
                            <MenuItem value="all">Show all</MenuItem>
                            {ODLC_COLORS.map((c) => (
                                <MenuItem key={c} value={c}>
                                    {ODLC_COLOR_LABELS[c]}
                                </MenuItem>
                            ))}
                        </TextField>
                    </Stack>

                    <Typography variant="subtitle2" color="text.secondary" gutterBottom>
                        Images
                    </Typography>
                    <Box ref={listContainerCallbackRef} sx={{ flex: 1, minHeight: 0, overflow: "hidden" }}>
                        <Stack spacing={0.25}>
                            {paginatedRecords.map((record) => (
                                <Box
                                    key={record.id}
                                    onClick={() => handleSelect(record.id)}
                                    sx={{
                                        display: "flex",
                                        alignItems: "center",
                                        gap: 1,
                                        p: 0.75,
                                        borderRadius: 1,
                                        cursor: "pointer",
                                        border: "1px solid",
                                        borderColor: selectedId === record.id ? "primary.main" : "transparent",
                                        bgcolor: selectedId === record.id ? "action.selected" : "transparent",
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
                                        {new Date(record.receivedAt).toLocaleTimeString()} ·{" "}
                                        {record.image.confidence_level}
                                    </Typography>
                                    {record.flagged && <Flag sx={{ fontSize: 14, color: "warning.main" }} />}
                                </Box>
                            ))}
                            {paginatedRecords.length === 0 && (
                                <Typography variant="caption" color="text.secondary" sx={{ px: 1, py: 0.5 }}>
                                    No images match current filters.
                                </Typography>
                            )}
                        </Stack>
                    </Box>

                    {filteredSorted.length > pageSize && (
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

                {/* Main panel */}
                <Paper sx={{ flex: 1, p: 2, display: "flex", flexDirection: "column", minWidth: 0 }}>
                    <Box sx={{ display: "flex", justifyContent: "flex-end", mb: 1 }}>
                        <Button
                            variant="outlined"
                            size="small"
                            disabled={flaggedForExport.length === 0}
                            onClick={handleExport}
                        >
                            Export
                        </Button>
                    </Box>
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
                        {records.length === 0 ? (
                            <Stack alignItems="center" spacing={1}>
                                <Typography color="text.secondary">No ODLC images captured yet.</Typography>
                                <Typography variant="caption" color="text.secondary">
                                    Connect to a WebRTC stream to receive images.
                                </Typography>
                                <Button variant="contained" size="small" onClick={loadSampleImages} sx={{ mt: 1 }}>
                                    Load sample images
                                </Button>
                            </Stack>
                        ) : selectedRecord ? (
                            <>
                                <ImageAnnotationOverlay
                                    imageSrc={`data:image/jpeg;base64,${selectedRecord.image.image_data}`}
                                    annotations={selectedRecord.annotations}
                                    recordId={selectedRecord.id}
                                    highlightedAnnotationId={highlightedAnnotationId}
                                    boundingBox={selectedRecord.image.bounding_box}
                                    depthData={selectedRecord.image.depth_data}
                                    onAddAnnotation={handleImageAnnotation}
                                    onUndo={(id) => dispatch(undoLastOdlcImageAnnotation(id))}
                                    onDeleteAnnotation={(args) => dispatch(deleteOdlcImageAnnotation(args))}
                                />
                                {selectedRecord.metadata.trim() && (
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
                                            {selectedRecord.metadata.trim()}
                                        </Typography>
                                    </Box>
                                )}
                            </>
                        ) : (
                            <Typography color="text.secondary">Select an image</Typography>
                        )}
                    </Box>

                    {selectedRecord && (
                        <Stack spacing={1.5} sx={{ mt: 2 }}>
                            <TextField
                                label="Text input"
                                size="small"
                                fullWidth
                                multiline
                                minRows={2}
                                value={selectedRecord.textInput}
                                onFocus={() => {
                                    textInputFocusValueRef.current = selectedRecord.textInput;
                                }}
                                onBlur={() => {
                                    const initial = textInputFocusValueRef.current;
                                    textInputFocusValueRef.current = null;
                                    if (initial !== null && initial !== selectedRecord.textInput) {
                                        dispatch(syncOdlcImageToBackend(selectedRecord.id));
                                    }
                                }}
                                onChange={(e) => {
                                    const next = e.target.value;
                                    const wasEmpty = selectedRecord.textInput.trim() === "";
                                    const nowEmpty = next.trim() === "";
                                    if (wasEmpty && !nowEmpty) {
                                        dispatch(
                                            updateOdlcImageFlag({
                                                id: selectedRecord.id,
                                                flagged: true,
                                            }),
                                        );
                                    }
                                    if (!wasEmpty && nowEmpty) {
                                        dispatch(
                                            updateOdlcImageFlag({
                                                id: selectedRecord.id,
                                                flagged: false,
                                            }),
                                        );
                                    }
                                    dispatch(
                                        updateOdlcImageTextInput({
                                            id: selectedRecord.id,
                                            textInput: next,
                                        }),
                                    );
                                }}
                            />
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
                                    {formatImageMetadata(selectedRecord)}
                                </Box>
                            </Box>
                            <Box>
                                <Button
                                    variant={selectedRecord.flagged ? "contained" : "outlined"}
                                    size="small"
                                    startIcon={<Flag />}
                                    onClick={() =>
                                        dispatch(
                                            updateOdlcImageFlag({
                                                id: selectedRecord.id,
                                                flagged: !selectedRecord.flagged,
                                            }),
                                        )
                                    }
                                >
                                    {selectedRecord.flagged ? "Flagged" : "Flag"}
                                </Button>
                            </Box>
                        </Stack>
                    )}
                </Paper>
            </Box>
        </Box>
    );
}
