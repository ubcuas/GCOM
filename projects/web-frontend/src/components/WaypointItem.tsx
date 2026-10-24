import { Box, IconButton, Paper, Stack, SxProps, Typography } from "@mui/material";
import { Waypoint } from "../types/Waypoint";
import DeleteIcon from "@mui/icons-material/Delete";
import EditIcon from "@mui/icons-material/Edit";

type WaypointItemProps = {
    waypoint: Waypoint;
    sx?: SxProps;
    handleDelete?: () => void;
    handleEdit?: (wp: Waypoint) => void;
};

export default function WaypointItem({ waypoint, sx, handleDelete, handleEdit }: WaypointItemProps) {
    return (
        <Paper elevation={4} sx={{ ...sx, p: 1 }}>
            <Stack direction="row" alignItems="center" justifyContent="space-between">
                <Typography variant="h6">{waypoint.name || "No Name"}</Typography>
                <Stack direction="row" alignItems="center" justifyContent="space-between">
                    <Typography color="grey">
                        ID#{waypoint.id.length > 4 ? `${waypoint.id.slice(0, 4)}...` : waypoint.id}
                    </Typography>
                    <Box>
                        {handleDelete && (
                            <IconButton color="warning" size="medium" onClick={handleDelete}>
                                <DeleteIcon fontSize="inherit" />
                            </IconButton>
                        )}
                        {handleEdit && (
                            <IconButton color="primary" size="medium" onClick={() => handleEdit(waypoint)}>
                                <EditIcon fontSize="inherit" />
                            </IconButton>
                        )}
                    </Box>
                </Stack>
            </Stack>
            <Typography variant="body1">
                Latitude{" "}
                <Typography
                    component="span"
                    sx={{
                        color: (theme) => theme.palette.primary.main,
                    }}
                >
                    {waypoint.latitude}
                </Typography>
            </Typography>
            <Typography variant="body1">
                Longitude{" "}
                <Typography
                    component="span"
                    sx={{
                        color: (theme) => theme.palette.primary.main,
                    }}
                >
                    {waypoint.longitude}
                </Typography>
            </Typography>
            <Typography variant="body1">
                Altitude{" "}
                <Typography
                    component="span"
                    sx={{
                        color: (theme) => theme.palette.primary.main,
                    }}
                >
                    {waypoint.altitude}
                </Typography>
            </Typography>
            {waypoint.command && (
                <Typography variant="body1">
                    Command{" "}
                    <Typography
                        component="span"
                        sx={{
                            color: (theme) => theme.palette.primary.main,
                        }}
                    >
                        {waypoint.command}
                    </Typography>
                </Typography>
            )}
            {waypoint.param1 && (
                <Typography variant="body1">
                    Param1{" "}
                    <Typography
                        component="span"
                        sx={{
                            color: (theme) => theme.palette.primary.main,
                        }}
                    >
                        {waypoint.param1}
                    </Typography>
                </Typography>
            )}
            {waypoint.param2 && (
                <Typography variant="body1">
                    Param2{" "}
                    <Typography
                        component="span"
                        sx={{
                            color: (theme) => theme.palette.primary.main,
                        }}
                    >
                        {waypoint.param2}
                    </Typography>
                </Typography>
            )}
            {waypoint.param3 && (
                <Typography variant="body1">
                    Param3{" "}
                    <Typography
                        component="span"
                        sx={{
                            color: (theme) => theme.palette.primary.main,
                        }}
                    >
                        {waypoint.param3}
                    </Typography>
                </Typography>
            )}
            {waypoint.param4 && (
                <Typography variant="body1">
                    Param4{" "}
                    <Typography
                        component="span"
                        sx={{
                            color: (theme) => theme.palette.primary.main,
                        }}
                    >
                        {waypoint.param4}
                    </Typography>
                </Typography>
            )}
            {waypoint.remarks && <Typography color="grey">Remarks -- {waypoint.remarks}</Typography>}
        </Paper>
    );
}
