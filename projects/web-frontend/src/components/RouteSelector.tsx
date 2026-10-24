import { useEffect, useState } from "react";
import {
    Box,
    Button,
    FormControl,
    InputLabel,
    MenuItem,
    Modal,
    Paper,
    Select,
    TextField,
    Typography,
} from "@mui/material";
import { useAppDispatch, useAppSelector } from "../store/store";
import { selectAvailableRoutes, selectCurrentRoute } from "../store/slices/dataSlice";
import { ensureActiveRoute, switchToRoute, createNewRoute, deleteRouteById } from "../store/thunks/dataThunks";

export const RouteSelector = () => {
    const dispatch = useAppDispatch();
    const availableRoutes = useAppSelector(selectAvailableRoutes);
    const currentRoute = useAppSelector(selectCurrentRoute);
    const [showCreateDialog, setShowCreateDialog] = useState(false);
    const [newRouteName, setNewRouteName] = useState("");
    const [showDeleteConfirm, setShowDeleteConfirm] = useState(false);
    const [routeToDelete, setRouteToDelete] = useState<number | null>(null);

    useEffect(() => {
        void dispatch(ensureActiveRoute());
    }, [dispatch]);

    const handleRouteChange = (routeId: number) => {
        void dispatch(switchToRoute(routeId));
    };

    const handleCreateRoute = async () => {
        if (newRouteName.trim()) {
            const result = await dispatch(createNewRoute(newRouteName.trim()));
            if (result.meta.requestStatus === "fulfilled") {
                const newRoute = result.payload as { id: number };
                await dispatch(switchToRoute(newRoute.id));
            }
            setNewRouteName("");
            setShowCreateDialog(false);
        }
    };

    const handleDeleteClick = (routeId: number) => {
        setRouteToDelete(routeId);
        setShowDeleteConfirm(true);
    };

    const handleConfirmDelete = async () => {
        if (routeToDelete !== null) {
            await dispatch(deleteRouteById(routeToDelete));
            setRouteToDelete(null);
            setShowDeleteConfirm(false);
            void dispatch(ensureActiveRoute());
        }
    };

    return (
        <Box
            sx={{
                display: "flex",
                flexDirection: "column",
                gap: 2,
            }}
        >
            <Box
                sx={{
                    display: "flex",
                    justifyContent: "space-between",
                    alignItems: "center",
                }}
            >
                <Typography variant="h6">Routes</Typography>
                <Button variant="outlined" onClick={() => setShowCreateDialog(true)} size="small">
                    + New Route
                </Button>
            </Box>

            <Box
                sx={{
                    display: "flex",
                    flexDirection: "column",
                    gap: 2,
                }}
            >
                <FormControl size="small" fullWidth>
                    <InputLabel id="route-select-label">Select a route</InputLabel>
                    <Select
                        labelId="route-select-label"
                        value={currentRoute ? String(currentRoute.id) : ""}
                        onChange={(e) => handleRouteChange(Number(e.target.value))}
                        label="Select a route"
                    >
                        {availableRoutes.map((route) => (
                            <MenuItem key={route.id} value={route.id}>
                                {route.name} ({route.waypoints.length} waypoints)
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>

                {currentRoute && (
                    <Box
                        sx={{
                            display: "flex",
                            justifyContent: "space-between",
                            alignItems: "center",
                            gap: 2,
                        }}
                    >
                        <Typography variant="body2">Active: {currentRoute.name}</Typography>
                        <Button
                            variant="outlined"
                            color="error"
                            size="small"
                            onClick={() => handleDeleteClick(currentRoute.id)}
                        >
                            Delete
                        </Button>
                    </Box>
                )}
            </Box>

            <Modal open={showCreateDialog} onClose={() => setShowCreateDialog(false)}>
                <Paper
                    elevation={2}
                    sx={{
                        position: "absolute",
                        top: "50%",
                        left: "50%",
                        transform: "translate(-50%, -50%)",
                        p: 4,
                        minWidth: 300,
                        display: "flex",
                        flexDirection: "column",
                        gap: 2,
                    }}
                >
                    <Typography variant="h6">Create New Route</Typography>
                    <TextField
                        size="small"
                        fullWidth
                        label="Route name"
                        value={newRouteName}
                        onChange={(e) => setNewRouteName(e.target.value)}
                        autoFocus
                        onKeyPress={(e) => {
                            if (e.key === "Enter" && newRouteName.trim()) {
                                handleCreateRoute();
                            }
                        }}
                    />
                    <Box sx={{ display: "flex", gap: 2, justifyContent: "flex-end" }}>
                        <Button variant="outlined" onClick={() => setShowCreateDialog(false)}>
                            Cancel
                        </Button>
                        <Button variant="contained" onClick={handleCreateRoute} disabled={!newRouteName.trim()}>
                            Create
                        </Button>
                    </Box>
                </Paper>
            </Modal>

            <Modal open={showDeleteConfirm} onClose={() => setShowDeleteConfirm(false)}>
                <Paper
                    elevation={2}
                    sx={{
                        position: "absolute",
                        top: "50%",
                        left: "50%",
                        transform: "translate(-50%, -50%)",
                        p: 4,
                        minWidth: 300,
                        display: "flex",
                        flexDirection: "column",
                        gap: 2,
                    }}
                >
                    <Typography variant="h6">Confirm Delete</Typography>
                    <Typography variant="body2">
                        Are you sure you want to delete this route? This action cannot be undone.
                    </Typography>
                    <Box sx={{ display: "flex", gap: 2, justifyContent: "flex-end" }}>
                        <Button variant="outlined" onClick={() => setShowDeleteConfirm(false)}>
                            Cancel
                        </Button>
                        <Button variant="contained" color="error" onClick={handleConfirmDelete}>
                            Delete
                        </Button>
                    </Box>
                </Paper>
            </Modal>
        </Box>
    );
};
