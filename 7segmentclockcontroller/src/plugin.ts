import streamDeck, { LogLevel } from "@elgato/streamdeck";

import { ClockColorControl } from "./actions/increment-counter";
import { ClockTransitionControl } from "./actions/transition-control";
import { ClockSetTimeControl } from "./actions/set-time-control";
import { ClockStopControl } from "./actions/stop";
import { ClockStartControl } from "./actions/start";

// We can enable "trace" logging so that all messages between the Stream Deck, and the plugin are recorded. When storing sensitive information
streamDeck.logger.setLevel(LogLevel.TRACE);

// Register the increment action.
streamDeck.actions.registerAction(new ClockColorControl());
streamDeck.actions.registerAction(new ClockTransitionControl());
streamDeck.actions.registerAction(new ClockSetTimeControl());
streamDeck.actions.registerAction(new ClockStopControl());
streamDeck.actions.registerAction(new ClockStartControl());

// Finally, connect to the Stream Deck.
streamDeck.connect();
