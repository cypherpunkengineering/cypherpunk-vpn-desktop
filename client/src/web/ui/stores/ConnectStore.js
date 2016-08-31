import { EventEmitter } from "events";
import dispatcher from "../dispatcher/dispatcher";

class ConnectStore extends EventEmitter {
  constructor() {
    super();
    this.vpn = { connectState : 'disconnected' };
  }

  getState() {
    return this.vpn["connectState"];
  }

  updateConnect(connectState){
    this.vpn["connectState"] = connectState;
    this.emit("change");
  }

  handleActions(action) {
    switch(action.type) {
      case "UPDATE" :{
        this.updateConnect(action.connectState);
      }
    }
  }
  
}

const connectStore = new ConnectStore;
dispatcher.register(connectStore.handleActions.bind(connectStore));
export default connectStore;
