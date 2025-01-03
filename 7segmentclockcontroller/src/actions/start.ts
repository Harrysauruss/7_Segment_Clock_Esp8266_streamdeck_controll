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
 * Action to start the ESP8266 LED Clock
 */
@action({ UUID: "com.marius.7segmentclockcontroller.start" })
export class ClockStartControl extends SingletonAction<StartSettings> {
    /**
     * Updates the key's image with a play icon
     */
    private async updateKeyImage(action: StreamDeckAction): Promise<void> {
        // Create an SVG play icon (triangle)
        const svg = `<svg width="144" height="144" viewBox="0 0 144 144">
            <polygon points="42,32 112,72 42,112" fill="rgb(50,255,50)"/>
        </svg>`;

        await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
    }

    /**
     * Called when the action appears on the Stream Deck
     */
    override async onWillAppear(ev: WillAppearEvent<StartSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }

    /**
     * Called when the action key is pressed
     */
    override async onKeyDown(ev: KeyDownEvent<StartSettings>): Promise<void> {
        const settings = ev.payload.settings;
        
        // Validate IP address
        if (!settings.espIP) {
            await ev.action.showAlert();
            streamDeck.logger.error('ESP8266 IP address not configured');
            return;
        }

        try {
            // Make HTTP request to ESP8266
            const response = await fetch(`http://${settings.espIP}/start`);
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            await ev.action.showOk();
        } catch (error) {
            await ev.action.showAlert();
            streamDeck.logger.error(`Failed to start clock on ESP8266: ${error}`);
        }
    }

    /**
     * Called when settings are updated
     */
    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<StartSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }
}

/**
 * Settings for ClockStartControl
 */
type StartSettings = {
    espIP?: string;
};
