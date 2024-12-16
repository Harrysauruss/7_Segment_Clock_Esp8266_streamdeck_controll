import { 
    action, 
    KeyDownEvent, 
    SingletonAction, 
    WillAppearEvent,
    DidReceiveSettingsEvent 
} from "@elgato/streamdeck";
import streamDeck from "@elgato/streamdeck";

/**
 * Action to directly set ESP8266 LED Clock time
 */
@action({ UUID: "com.marius.7segmentclockcontroller.settime" })
export class ClockSetTimeControl extends SingletonAction<SetTimeSettings> {
    /**
     * Updates the key's image with a clock icon
     */
    private async updateKeyImage(action: StreamDeckAction): Promise<void> {
        // Create an SVG clock icon
        const svg = `<svg width="144" height="144" viewBox="0 0 144 144">
            <circle cx="72" cy="72" r="60" fill="none" stroke="rgb(200,200,200)" stroke-width="8"/>
            <line x1="72" y1="72" x2="72" y2="30" stroke="rgb(0,150,255)" stroke-width="8" stroke-linecap="round"/>
            <line x1="72" y1="72" x2="100" y2="72" stroke="rgb(0,150,255)" stroke-width="8" stroke-linecap="round"/>
        </svg>`;

        await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
    }

    /**
     * Called when the action appears on the Stream Deck
     */
    override async onWillAppear(ev: WillAppearEvent<SetTimeSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }

    /**
     * Called when the action key is pressed
     */
    override async onKeyDown(ev: KeyDownEvent<SetTimeSettings>): Promise<void> {
        const settings = ev.payload.settings;
        
        // Validate IP address
        if (!settings.espIP) {
            await ev.action.showAlert();
            streamDeck.logger.error('ESP8266 IP address not configured');
            return;
        }

        // Set default values if not configured
        const hours = settings.hours ?? 0;
        const minutes = settings.minutes ?? 0;

        try {
            // Make HTTP request to ESP8266
            const response = await fetch(
                `http://${settings.espIP}/set?h=${hours}&m=${minutes}`
            );
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            await ev.action.showOk();
        } catch (error) {
            await ev.action.showAlert();
            streamDeck.logger.error(`Failed to set time on ESP8266: ${error}`);
        }
    }

    /**
     * Called when settings are updated
     */
    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<SetTimeSettings>): Promise<void> {
        await this.updateKeyImage(ev.action);
    }
}

/**
 * Settings for ClockSetTimeControl
 */
type SetTimeSettings = {
    espIP?: string;
    hours?: number;
    minutes?: number;
}; 