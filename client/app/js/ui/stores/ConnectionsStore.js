import { EventEmitter } from "events";

class ConnectionsStore extends EventEmitter{
  constructor() {
    super();
    this.connection_state = 'disconnected';
  }

  getState() {
    return this.connection_state;
  }
}

const connectionsStore = new ConnectionsStore;

export default connectionsStore;
