import streamDeck, { LogLevel } from "@elgato/streamdeck";

import { ClockColorControl } from "./actions/increment-counter";
import { ClockTransitionControl } from "./actions/transition-control";
import { ClockSetTimeControl } from "./actions/set-time-control";

// We can enable "trace" logging so that all messages between the Stream Deck, and the plugin are recorded. When storing sensitive information
streamDeck.logger.setLevel(LogLevel.TRACE);
streamDeck.logger.debug("Starting plugin");

streamDeck.settings.setGlobalSettings({
    espIP: "192.168.1.100"
});

// Register the increment action.
streamDeck.actions.registerAction(new ClockColorControl());
streamDeck.actions.registerAction(new ClockTransitionControl());
streamDeck.actions.registerAction(new ClockSetTimeControl());

// Finally, connect to the Stream Deck.
streamDeck.connect();
