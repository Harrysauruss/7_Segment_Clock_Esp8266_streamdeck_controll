import streamDeck, { LogLevel } from "@elgato/streamdeck";
import { ClockController } from "./actions/clock-controller";

// We can enable "trace" logging so that all messages between the Stream Deck, and the plugin are recorded. When storing sensitive information
streamDeck.logger.setLevel(LogLevel.TRACE);

// Register all actions
streamDeck.actions.registerAction(new ClockController());

// Finally, connect to the Stream Deck.
streamDeck.connect();
