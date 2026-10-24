import { deprojectPixel } from "../api/endpoints";
import { decode } from "fast-png";

export type DecodedDepthImage = { data: Uint16Array; width: number; height: number };

/**
 * Decodes a base64-encoded 16-bit grayscale PNG (ROS2 CompressedImage 16UC1 format)
 * into a flat Uint16Array of depth values in millimeters, plus image dimensions.
 **/

export function decodeDepthPng(base64: string): DecodedDepthImage {
    const binaryStr = atob(base64);
    const bytes = new Uint8Array(binaryStr.length);
    for (let i = 0; i < binaryStr.length; i++) {
        bytes[i] = binaryStr.charCodeAt(i);
    }

    const png = decode(bytes);
    const data = png.data as Uint16Array;

    return { data, width: png.width, height: png.height };
}

/**
 * Samples depth (in mm) at a normalized 0-1 coordinate from a decoded 16UC1 depth map.
 * Uses a 5x5 neighbourhood, then discards outlier readings that differ from the median
 * by more than 10% before averaging — this rejects edge-bleed and specular spikes while
 * still smoothing over sensor noise.
 */
function sampleDepth(data: Uint16Array, width: number, height: number, nx: number, ny: number): number {
    const px = Math.floor(nx * width);
    const py = Math.floor(ny * height);

    const radius = 2; // 5x5 window
    const readings: number[] = [];

    for (let dy = -radius; dy <= radius; dy++) {
        for (let dx = -radius; dx <= radius; dx++) {
            const x = Math.max(0, Math.min(width - 1, px + dx));
            const y = Math.max(0, Math.min(height - 1, py + dy));
            const val = data[y * width + x];
            if (val > 0) readings.push(val);
        }
    }

    if (readings.length === 0) return 0;

    // Find the median
    const sorted = [...readings].sort((a, b) => a - b);
    const median = sorted[Math.floor(sorted.length / 2)];

    // Average only readings within 10% of the median (discard outliers)
    const threshold = median * 0.1;
    let sum = 0;
    let count = 0;
    for (const v of readings) {
        if (Math.abs(v - median) <= threshold) {
            sum += v;
            count++;
        }
    }

    return count > 0 ? sum / count : median;
}

export function sampleDepthMeters(
    decodedDepth: DecodedDepthImage | null,
    point: { x: number; y: number },
): number | null {
    if (!decodedDepth) return null;

    const depthMm = sampleDepth(decodedDepth.data, decodedDepth.width, decodedDepth.height, point.x, point.y);
    return depthMm > 0 ? depthMm / 1000 : null;
}

export type CameraPoint3D = [number, number, number];

export type AnnotationMeasurement = {
    distance: number;
    p1_3d: CameraPoint3D;
    p2_3d: CameraPoint3D;
};

/**
 * Computes 3D measurements (in meters) for two annotation points drawn on a 2D
 * ODLC image, using the associated depth map and camera intrinsics.
 *
 * Process:
 *  1. Decodes the base64-encoded 16UC1 PNG depth map into a Uint16Array of mm values
 *  2. Samples depth at each normalized (0-1) point coordinate via nearest-neighbor
 *  3. Calls the backend /vision/deproject_pixel/ for each point to get
 *     3D camera-space coordinates [x, y, z] in meters
 *  4. Returns those 3D points and their Euclidean distance.
 *     Callers can derive dX/dY/dZ in meters from the two points.
 *
 * Returns null if depth_data is unavailable or either sampled depth is zero
 * (meaning the depth sensor returned no reading at that pixel).
 */
export const calculateAnnotationDistance = async (
    p1: { x: number; y: number },
    p2: { x: number; y: number },
    depthData: string | null,
): Promise<AnnotationMeasurement | null> => {
    if (!depthData) return null;

    const { data, width, height } = decodeDepthPng(depthData);
    console.log("Depth data:", data);

    const d1Mm = sampleDepth(data, width, height, p1.x, p1.y);
    const d2Mm = sampleDepth(data, width, height, p2.x, p2.y);
    console.log("Sampled depths (mm):", d1Mm, d2Mm);

    if (d1Mm === 0 || d2Mm === 0) {
        console.warn("Distance calculation failed: one or both sampled depths are zero.");
        return null;
    }

    try {
        const [res1, res2] = await Promise.all([
            deprojectPixel([p1.x * width, p1.y * height], d1Mm / 1000),
            deprojectPixel([p2.x * width, p2.y * height], d2Mm / 1000),
        ]);
        console.log("Deprojection results:", res1, res2);

        const p1_3d: CameraPoint3D = [res1.point[0], res1.point[1], res1.point[2]];
        const p2_3d: CameraPoint3D = [res2.point[0], res2.point[1], res2.point[2]];

        const dist = Math.sqrt((p2_3d[0] - p1_3d[0]) ** 2 + (p2_3d[1] - p1_3d[1]) ** 2 + (p2_3d[2] - p1_3d[2]) ** 2);
        console.log("Calculated distance (m):", dist);
        return { distance: dist, p1_3d, p2_3d };
    } catch (error) {
        console.error("Distance calculation failed during API call:", error);
        return null;
    }
};
