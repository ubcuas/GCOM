import { Box, IconButton, Slider, Typography } from "@mui/material";
import DragIndicator from "@mui/icons-material/DragIndicator";
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import { decode } from "fast-png";
import { useCallback, useEffect, useRef, useState } from "react";
import type { OdlcImageAnnotation } from "../store/slices/dataSlice";

function useImageOverlayRect(imageRef: React.RefObject<HTMLImageElement | null>, imageSrc: string) {
    const [rect, setRect] = useState<{ left: number; top: number; width: number; height: number } | null>(null);
    const parentRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const img = imageRef.current;
        const parent = parentRef.current;
        if (!img || !parent) return;
        const update = () => {
            const pr = parent.getBoundingClientRect();
            const ir = img.getBoundingClientRect();
            setRect({
                left: ir.left - pr.left,
                top: ir.top - pr.top,
                width: ir.width,
                height: ir.height,
            });
        };
        update();
        img.addEventListener("load", update);
        const ro = new ResizeObserver(update);
        ro.observe(parent);
        return () => {
            img.removeEventListener("load", update);
            ro.disconnect();
        };
    }, [imageSrc, imageRef]);

    return { rect, parentRef };
}

const HIT_THRESHOLD = 0.02; // normalized units
const DEFAULT_ANNOTATION_STROKE_WIDTH = 0.005;
const DEFAULT_ANNOTATION_STROKE_COLOR = "red";
const HIGHLIGHTED_ANNOTATION_STROKE_COLOR = "yellow";

function hitTestSegment(nx: number, ny: number, p1: { x: number; y: number }, p2: { x: number; y: number }): number {
    const dx = p2.x - p1.x;
    const dy = p2.y - p1.y;
    const len = Math.hypot(dx, dy) || 1e-9;
    const t = Math.max(0, Math.min(1, ((nx - p1.x) * dx + (ny - p1.y) * dy) / (len * len)));
    const px = p1.x + t * dx;
    const py = p1.y + t * dy;
    return Math.hypot(nx - px, ny - py);
}

function decodeDepth(base64: string): { data: Uint16Array; width: number; height: number } {
    const binaryStr = atob(base64);
    const bytes = new Uint8Array(binaryStr.length);
    for (let i = 0; i < binaryStr.length; i++) {
        bytes[i] = binaryStr.charCodeAt(i);
    }
    const png = decode(bytes);
    return { data: png.data as Uint16Array, width: png.width, height: png.height };
}

/** Jet colormap: t=0 → blue, t=0.5 → green/yellow, t=1 → red. */
function jetColormap(t: number): [number, number, number] {
    const r = Math.max(0, Math.min(255, Math.round(255 * (1.5 - Math.abs(4 * t - 3)))));
    const g = Math.max(0, Math.min(255, Math.round(255 * (1.5 - Math.abs(4 * t - 2)))));
    const b = Math.max(0, Math.min(255, Math.round(255 * (1.5 - Math.abs(4 * t - 1)))));
    return [r, g, b];
}

/** Bounding box as array of [x, y] in normalized 0-1 coords. Two points define the rectangle. */
export type BoundingBox = Array<[number, number]>;

type ImageAnnotationOverlayProps = {
    imageSrc: string;
    annotations: OdlcImageAnnotation[];
    recordId: string;
    highlightedAnnotationId?: string | null;
    /** Image bounding box (e.g. from ODLC image data); drawn as a rectangle from the first two points. */
    boundingBox?: BoundingBox;
    /** Base64-encoded 16UC1 PNG depth map, or null if unavailable. */
    depthData?: string | null;
    onAddAnnotation: (args: {
        id: string;
        annotationId: string;
        p1: { x: number; y: number };
        p2: { x: number; y: number };
    }) => void;
    onUndo: (id: string) => void;
    onDeleteAnnotation: (args: { id: string; annotationId: string }) => void;
    instructions?: string;
};

export default function ImageAnnotationOverlay({
    imageSrc,
    annotations,
    recordId,
    highlightedAnnotationId = null,
    boundingBox,
    depthData,
    onAddAnnotation,
    onUndo,
    onDeleteAnnotation,
    instructions,
}: ImageAnnotationOverlayProps) {
    const imageRef = useRef<HTMLImageElement>(null);
    const { rect: overlayRect, parentRef } = useImageOverlayRect(imageRef, imageSrc);
    const [dragging, setDragging] = useState<{ p1: { x: number; y: number }; p2: { x: number; y: number } } | null>(
        null,
    );

    // Depth map state
    const canvasRef = useRef<HTMLCanvasElement>(null);
    const [depthOpacity, setDepthOpacity] = useState(0.5);
    const [depthSliderVisible, setDepthSliderVisible] = useState(true);
    const [depthDecoded, setDepthDecoded] = useState<{ data: Uint16Array; width: number; height: number } | null>(null);

    // Drag state for the depth control bar
    const [dragDelta, setDragDelta] = useState({ x: 0, y: 0 });
    const dragRef = useRef<{ mx: number; my: number; dx: number; dy: number } | null>(null);

    const handleDragStart = (e: React.PointerEvent) => {
        e.stopPropagation();
        dragRef.current = { mx: e.clientX, my: e.clientY, dx: dragDelta.x, dy: dragDelta.y };
        (e.currentTarget as HTMLElement).setPointerCapture(e.pointerId);
    };

    const handleDragMove = (e: React.PointerEvent) => {
        if (!dragRef.current) return;
        e.stopPropagation();
        setDragDelta({
            x: dragRef.current.dx + e.clientX - dragRef.current.mx,
            y: dragRef.current.dy + e.clientY - dragRef.current.my,
        });
    };

    const handleDragEnd = (e: React.PointerEvent) => {
        e.stopPropagation();
        dragRef.current = null;
    };

    useEffect(() => {
        if (!depthData) {
            setDepthDecoded(null);
            return;
        }
        try {
            setDepthDecoded(decodeDepth(depthData));
        } catch (e) {
            console.error("Failed to decode depth data:", e);
            setDepthDecoded(null);
        }
    }, [depthData]);

    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas || !depthDecoded || !overlayRect) return;

        const w = Math.round(overlayRect.width);
        const h = Math.round(overlayRect.height);
        canvas.width = w;
        canvas.height = h;

        const ctx = canvas.getContext("2d");
        if (!ctx) return;

        const { data, width, height } = depthDecoded;

        let min = Infinity;
        let max = 0;
        for (const v of data) {
            if (v > 0) {
                if (v < min) min = v;
                if (v > max) max = v;
            }
        }
        if (!isFinite(min) || max === min) {
            ctx.clearRect(0, 0, w, h);
            return;
        }

        const range = max - min;
        const imgData = ctx.createImageData(w, h);
        for (let py = 0; py < h; py++) {
            for (let px = 0; px < w; px++) {
                const sx = Math.floor((px / w) * width);
                const sy = Math.floor((py / h) * height);
                const val = data[sy * width + sx];
                const i = (py * w + px) * 4;
                if (val === 0) {
                    imgData.data[i + 3] = 0; // transparent for missing depth
                } else {
                    const t = (val - min) / range;
                    const [r, g, b] = jetColormap(t);
                    imgData.data[i] = r;
                    imgData.data[i + 1] = g;
                    imgData.data[i + 2] = b;
                    imgData.data[i + 3] = 255;
                }
            }
        }
        ctx.putImageData(imgData, 0, 0);
    }, [depthDecoded, overlayRect]);

    const getNormalizedCoords = useCallback((clientX: number, clientY: number): { x: number; y: number } | null => {
        const el = imageRef.current;
        if (!el) return null;
        const rect = el.getBoundingClientRect();
        const x = (clientX - rect.left) / rect.width;
        const y = (clientY - rect.top) / rect.height;
        return { x, y };
    }, []);

    const handlePointerDown = useCallback(
        (e: React.PointerEvent) => {
            if (e.button !== 0) return;
            const p = getNormalizedCoords(e.clientX, e.clientY);
            if (p) setDragging({ p1: p, p2: p });
        },
        [getNormalizedCoords],
    );

    const handlePointerMove = useCallback(
        (e: React.PointerEvent) => {
            if (!dragging) return;
            const p = getNormalizedCoords(e.clientX, e.clientY);
            if (p) setDragging((d) => (d ? { ...d, p2: p } : null));
        },
        [dragging, getNormalizedCoords],
    );

    const clamp = useCallback((x: number) => Math.max(0, Math.min(1, x)), []);

    const handlePointerUp = useCallback(
        (e: React.PointerEvent) => {
            if (e.button !== 0 || !dragging) return;
            const p = getNormalizedCoords(e.clientX, e.clientY);
            if (p) {
                const p1 = { x: clamp(dragging.p1.x), y: clamp(dragging.p1.y) };
                const p2 = { x: clamp(p.x), y: clamp(p.y) };
                const dist = Math.hypot(p2.x - p1.x, p2.y - p1.y);
                if (dist >= 0.005) {
                    onAddAnnotation({
                        id: recordId,
                        annotationId: crypto.randomUUID(),
                        p1,
                        p2,
                    });
                }
            }
            setDragging(null);
        },
        [dragging, getNormalizedCoords, recordId, onAddAnnotation, clamp],
    );

    const handleDoubleClick = useCallback(
        (e: React.MouseEvent) => {
            const p = getNormalizedCoords(e.clientX, e.clientY);
            if (!p) return;
            let best: { id: string; d: number } | null = null;
            for (const a of annotations) {
                const d = hitTestSegment(p.x, p.y, a.p1, a.p2);
                if (d < HIT_THRESHOLD && (!best || d < best.d)) best = { id: a.id, d };
            }
            if (best) onDeleteAnnotation({ id: recordId, annotationId: best.id });
        },
        [annotations, getNormalizedCoords, recordId, onDeleteAnnotation],
    );

    useEffect(() => {
        const onKeyDown = (e: KeyboardEvent) => {
            if (e.key === "z" && (e.ctrlKey || e.metaKey)) {
                e.preventDefault();
                onUndo(recordId);
            }
        };
        window.addEventListener("keydown", onKeyDown);
        return () => window.removeEventListener("keydown", onKeyDown);
    }, [recordId, onUndo]);

    return (
        <Box
            ref={parentRef}
            onPointerDown={handlePointerDown}
            onPointerMove={handlePointerMove}
            onPointerUp={handlePointerUp}
            onPointerLeave={() => setDragging(null)}
            onDoubleClick={handleDoubleClick}
            sx={{
                position: "relative",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                maxWidth: "100%",
                maxHeight: "100%",
                cursor: "crosshair",
            }}
        >
            <Box
                component="img"
                ref={imageRef}
                src={imageSrc}
                alt="Annotated"
                draggable={false}
                sx={{
                    display: "block",
                    maxWidth: "100%",
                    maxHeight: "100%",
                    objectFit: "contain",
                    userSelect: "none",
                    pointerEvents: "none",
                }}
            />
            {/* Depth map color overlay */}
            {depthDecoded && overlayRect && (
                <canvas
                    ref={canvasRef}
                    style={{
                        position: "absolute",
                        left: overlayRect.left,
                        top: overlayRect.top,
                        width: overlayRect.width,
                        height: overlayRect.height,
                        opacity: depthOpacity,
                        pointerEvents: "none",
                    }}
                />
            )}
            {overlayRect && (
                <Box
                    component="svg"
                    viewBox="0 0 1 1"
                    preserveAspectRatio="none"
                    sx={{
                        position: "absolute",
                        left: overlayRect.left,
                        top: overlayRect.top,
                        width: overlayRect.width,
                        height: overlayRect.height,
                        pointerEvents: "none",
                    }}
                >
                    {boundingBox &&
                        boundingBox.length >= 2 &&
                        (() => {
                            const [[x0, y0], [x1, y1]] = boundingBox;
                            const minX = Math.min(x0, x1);
                            const minY = Math.min(y0, y1);
                            const w = Math.abs(x1 - x0) || 0.01;
                            const h = Math.abs(y1 - y0) || 0.01;
                            return (
                                <rect
                                    x={minX}
                                    y={minY}
                                    width={w}
                                    height={h}
                                    fill="none"
                                    stroke="blue"
                                    strokeWidth={0.012}
                                    strokeDasharray="0.02 0.015"
                                />
                            );
                        })()}
                    {annotations.map((a) => (
                        <g key={a.id}>
                            <line
                                x1={a.p1.x}
                                y1={a.p1.y}
                                x2={a.p2.x}
                                y2={a.p2.y}
                                stroke={
                                    a.id === highlightedAnnotationId
                                        ? HIGHLIGHTED_ANNOTATION_STROKE_COLOR
                                        : DEFAULT_ANNOTATION_STROKE_COLOR
                                }
                                strokeWidth={DEFAULT_ANNOTATION_STROKE_WIDTH}
                                strokeLinecap="round"
                            />
                        </g>
                    ))}
                    {dragging && (
                        <line
                            x1={dragging.p1.x}
                            y1={dragging.p1.y}
                            x2={dragging.p2.x}
                            y2={dragging.p2.y}
                            stroke={DEFAULT_ANNOTATION_STROKE_COLOR}
                            strokeWidth={DEFAULT_ANNOTATION_STROKE_WIDTH}
                            strokeDasharray="0.02 0.02"
                            strokeLinecap="round"
                        />
                    )}
                </Box>
            )}
            {instructions && (
                <Box
                    sx={{
                        position: "absolute",
                        top: 8,
                        left: 8,
                        right: 8,
                        fontSize: "0.75rem",
                        color: "text.secondary",
                        pointerEvents: "none",
                    }}
                >
                    {instructions}
                </Box>
            )}
            {/* Depth map opacity control — draggable */}
            {depthDecoded && (
                <Box
                    sx={{
                        position: "absolute",
                        bottom: 8,
                        left: 8,
                        transform: `translate(${dragDelta.x}px, ${dragDelta.y}px)`,
                        display: "flex",
                        alignItems: "center",
                        gap: 1,
                        bgcolor: "rgba(0,0,0,0.55)",
                        borderRadius: 1,
                        pl: 0.5,
                        pr: 1.5,
                        py: 0.75,
                        zIndex: 10,
                        pointerEvents: "auto",
                        userSelect: "none",
                        ...(depthSliderVisible && { minWidth: 200 }),
                    }}
                >
                    {/* Drag handle */}
                    <Box
                        onPointerDown={handleDragStart}
                        onPointerMove={handleDragMove}
                        onPointerUp={handleDragEnd}
                        sx={{
                            display: "flex",
                            alignItems: "center",
                            cursor: "grab",
                            "&:active": { cursor: "grabbing" },
                            color: "rgba(255,255,255,0.6)",
                            flexShrink: 0,
                        }}
                    >
                        <DragIndicator fontSize="small" />
                    </Box>
                    <Typography variant="caption" sx={{ color: "white", whiteSpace: "nowrap", flexShrink: 0 }}>
                        Depth
                    </Typography>
                    <IconButton
                        size="small"
                        onClick={() => setDepthSliderVisible((v) => !v)}
                        sx={{ color: "white", p: 0.25, flexShrink: 0 }}
                        aria-label={depthSliderVisible ? "Hide opacity slider" : "Show opacity slider"}
                    >
                        {depthSliderVisible ? <Visibility fontSize="small" /> : <VisibilityOff fontSize="small" />}
                    </IconButton>
                    {depthSliderVisible && (
                        <>
                            <Slider
                                size="small"
                                value={depthOpacity}
                                min={0}
                                max={1}
                                step={0.01}
                                onChange={(_, v) => setDepthOpacity(v as number)}
                                onPointerDown={(e) => e.stopPropagation()}
                                onPointerMove={(e) => e.stopPropagation()}
                                onPointerUp={(e) => e.stopPropagation()}
                                sx={{ color: "white", "& .MuiSlider-thumb": { width: 12, height: 12 } }}
                            />
                            <Typography
                                variant="caption"
                                sx={{ color: "white", whiteSpace: "nowrap", flexShrink: 0, minWidth: 30 }}
                            >
                                {Math.round(depthOpacity * 100)}%
                            </Typography>
                        </>
                    )}
                </Box>
            )}
        </Box>
    );
}
