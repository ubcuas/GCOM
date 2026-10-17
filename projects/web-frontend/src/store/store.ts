import { Action, Middleware, ThunkAction, configureStore } from "@reduxjs/toolkit";
// eslint-disable-next-line @typescript-eslint/no-restricted-imports
import { useDispatch, useSelector } from "react-redux";
import dataReducer from "./slices/dataSlice";
import appReducer from "./slices/appSlice";
import { odlcSyncMiddleware } from "./middleware/odlcSyncMiddleware";

const localStorageMiddleware: Middleware = (store) => (next) => (action) => {
    const result = next(action);
    const typedAction = action as Action<string>;

    // Store appSlice in localStorage on update.
    if (typedAction.type.startsWith("app/")) {
        localStorage.setItem("appSlice", JSON.stringify(store.getState().app));
    }

    return result;
};

const store = configureStore({
    reducer: {
        data: dataReducer,
        app: appReducer,
        // more TBD
    },
    middleware: (getDefaultMiddleware) => getDefaultMiddleware().concat(localStorageMiddleware, odlcSyncMiddleware),
});

export default store;

// https://redux.js.org/usage/usage-with-typescript#define-root-state-and-dispatch-types
// Using these types within the rest of the app (instead of default ones) accounts for additional middlewares.
export type AppDispatch = typeof store.dispatch;
export type RootState = ReturnType<typeof store.getState>;

// https://redux.js.org/usage/usage-with-typescript#type-checking-redux-thunks
export type AppThunk<ReturnType = void> = ThunkAction<ReturnType, RootState, unknown, Action<string>>;

// https://redux.js.org/usage/usage-with-typescript#define-typed-hooks
export const useAppDispatch = useDispatch.withTypes<AppDispatch>();
export const useAppSelector = useSelector.withTypes<RootState>();
