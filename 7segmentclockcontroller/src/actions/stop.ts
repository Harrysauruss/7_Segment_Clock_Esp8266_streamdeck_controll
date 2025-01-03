import { 
    action, 
    KeyDownEvent, 
    SingletonAction, 
    WillAppearEvent,
    DidReceiveSettingsEvent,
    StreamDeckAction 
} from "@elgato/streamdeck";
import streamDeck from "@elgato/streamdeck";

/**
 * Action to stop the ESP8266 LED Clock
 */
@action({ UUID: "com.marius.7segmentclockcontroller.stop" })
export class ClockStopControl extends SingletonAction<StopSettings> {
    /**
     * Updates the key's image with a stop icon
     */
    private async updateKeyImage(action: StreamDeckAction): Promise<void> {
        // Create an SVG stop icon (square)
        const svg = `<svg width="144" height="144" viewBox="0 0 144 144">
            <rect x="42" y="42" width="60" height="60" fill="rgb(255,50,50)"/>
        </svg>`;

        await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
    }

    /**
     * Called when the action appears on the Stream Deck
     */
    override async onWillAppear(ev: WillAppearEvent<StopSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }

    /**
     * Called when the action key is pressed
     */
    override async onKeyDown(ev: KeyDownEvent<StopSettings>): Promise<void> {
        const settings = ev.payload.settings;
        
        // Validate IP address
        if (!settings.espIP) {
            await ev.action.showAlert();
            streamDeck.logger.error('ESP8266 IP address not configured');
            return;
        }

        try {
            // Make HTTP request to ESP8266
            const response = await fetch(`http://${settings.espIP}/stop`);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            await ev.action.showOk();
        } catch (error) {
            await ev.action.showAlert();
            streamDeck.logger.error(`Failed to stop clock on ESP8266: ${error}`);
        }
    }

    /**
     * Called when settings are updated
     */
    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<StopSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }
}

/**
 * Settings for ClockStopControl
 */
type StopSettings = {
    espIP?: string;
};
