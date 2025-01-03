import { 
    action, 
    KeyDownEvent, 
    SingletonAction, 
    WillAppearEvent,
    DidReceiveSettingsEvent 
} from "@elgato/streamdeck";
import streamDeck from "@elgato/streamdeck";
import GlobalSettings from "../interfaces/GlobalSettings";

/**
 * Action to control ESP8266 LED Clock transition
 */
@action({ UUID: "com.marius.7segmentclockcontroller.transition" })
export class ClockTransitionControl extends SingletonAction<TransitionSettings> {
    private currentAction?: StreamDeckAction;

    /**
     * Updates the key's image with a transition icon
     */
    private async updateKeyImage(settings: TransitionSettings, action: StreamDeckAction): Promise<void> {
        // Create an SVG transition icon
        const svg = `<svg width="144" height="144" viewBox="0 0 144 144">
            <circle cx="72" cy="72" r="60" fill="none" stroke="rgb(200,200,200)" stroke-width="8"/>
            <path d="M72 30 A42 42 0 1 1 30 72" stroke="rgb(0,150,255)" stroke-width="8" fill="none"/>
            <polygon points="60,25 80,25 70,45" fill="rgb(0,150,255)"/>
        </svg>`;

        await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
    }

    /**
     * Called when the action appears on the Stream Deck
     */
    override async onWillAppear(ev: WillAppearEvent<TransitionSettings>): Promise<void> {
        const settings = ev.payload.settings;
        this.currentAction = ev.action;
        await this.updateKeyImage(settings, ev.action);
    }

    /**
     * Called when the action key is pressed
     */
    override async onKeyDown(ev: KeyDownEvent<TransitionSettings>): Promise<void> {
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
        const transitionTime = settings.transitionTime ?? 5000;

        try {
            // Make HTTP request to ESP8266
            const response = await fetch(
                `http://${settings.espIP}/transition?h=${hours}&m=${minutes}&t=${transitionTime}`
            );
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            await ev.action.showOk();
        } catch (error) {
            await ev.action.showAlert();
            streamDeck.logger.error(`Failed to send transition to ESP8266: ${error}`);
        }
    }

    /**
     * Called when settings are updated
     */
    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<TransitionSettings>): Promise<void> {
		const globalSettings = await streamDeck.settings.getGlobalSettings<GlobalSettings>();
		if (ev.payload.settings.espIP !== globalSettings.espIP) {
			await streamDeck.settings.setGlobalSettings({ espIP: ev.payload.settings.espIP });
		}
		await this.updateKeyImage(ev.payload.settings, ev.action);
	}
}

/**
 * Settings for ClockTransitionControl
 */
type TransitionSettings = {
    espIP?: string;
    hours?: number;
    minutes?: number;
    transitionTime?: number;
}; 