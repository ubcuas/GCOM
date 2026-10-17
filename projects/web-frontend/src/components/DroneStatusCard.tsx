import { useAppSelector } from "../store/store";
import { selectAircraftStatus } from "../store/slices/dataSlice";
import PositionSection from "./DroneStatus/PositionSection";
import SpeedSection from "./DroneStatus/SpeedSection";
import MPSControlSection from "./DroneStatus/MPSControlSection";
import StatusIndicators from "./DroneStatus/StatusIndicators";
import { AircraftStatus } from "../types/AircraftStatus";
import { Paper } from "@mui/material";
import { useMemo } from "react";

const roundValues = (data: AircraftStatus) => {
    return {
        timestamp: Math.round(data.timestamp),
        latitude: Math.round(data.latitude * 1000000) / 1000000,
        longitude: Math.round(data.longitude * 1000000) / 1000000,
        altitude: Math.round(data.altitude),
        relativeAltitude: Math.round(data.relativeAltitude),
        verticalSpeed: Math.round(data.verticalSpeed * 100) / 100,
        speed: Math.round(data.speed * 100) / 100,
        heading: Math.round(data.heading),
        voltage: Math.round((data.voltage / 1000) * 100) / 100,
        armed: data.armed,
        flightmode: data.flightmode,
    } satisfies AircraftStatus;
};

export default function DroneStatusCard() {
    const droneState = useAppSelector(selectAircraftStatus);
    const roundedState = useMemo(() => roundValues(droneState), [droneState]);

    return (
        <Paper
            sx={{
                p: 2,
                display: "flex",
                flexDirection: "column",
                gap: 4,
            }}
        >
            <StatusIndicators
                flightMode={roundedState.flightmode || "UNKNOWN"}
                armed={roundedState.armed}
                voltage={roundedState.voltage}
            />
            <PositionSection
                latitude={roundedState.latitude}
                longitude={roundedState.longitude}
                altitude={roundedState.altitude}
                relativeAltitude={roundedState.relativeAltitude}
                heading={roundedState.heading}
            />
            <SpeedSection speed={roundedState.speed} verticalSpeed={roundedState.verticalSpeed} />
            <MPSControlSection />
        </Paper>
    );
}
