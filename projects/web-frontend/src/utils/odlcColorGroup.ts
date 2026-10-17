/**
 * Classifies ODLC image color_detection [r,g,b] into one of 7 named buckets.
 * HawkeyeOS emits canonical sentinel RGBs per color class; any RGB that does
 * not match a sentinel (within a small tolerance) is bucketed as "unknown".
 */

export type RGB = [number, number, number];

export type OdlcColor = "red" | "yellow" | "green" | "blue" | "black" | "white" | "unknown";

export const ODLC_COLORS: OdlcColor[] = ["red", "yellow", "green", "blue", "black", "white", "unknown"];

export const ODLC_COLOR_LABELS: Record<OdlcColor, string> = {
    red: "Red",
    yellow: "Yellow",
    green: "Green",
    blue: "Blue",
    black: "Black",
    white: "White",
    unknown: "Unknown",
};

export function classifyOdlcColor(rgb: RGB): OdlcColor {
    const [r, g, b] = rgb;
    const eq = (a: number, x: number) => Math.abs(a - x) <= 3;
    const is = (R: number, G: number, B: number) => eq(r, R) && eq(g, G) && eq(b, B);

    if (is(255, 0, 0)) return "red";
    if (is(255, 255, 0)) return "yellow";
    if (is(0, 255, 0)) return "green";
    if (is(0, 0, 255)) return "blue";
    if (is(0, 0, 0)) return "black";
    if (is(255, 255, 255)) return "white";
    return "unknown";
}
