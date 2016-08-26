import dispatcher from "../dispatcher/dispatcher";

export function updateConnect(connectState) {
  dispatcher.dispatch({
    type: "UPDATE",
    connectState: connectState
  });
}
