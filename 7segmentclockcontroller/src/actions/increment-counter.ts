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
 * Action to control ESP8266 LED Clock color
 */
@action({ UUID: "com.marius.7segmentclockcontroller.increment" })
export class ClockColorControl extends SingletonAction<ClockSettings> {
	private currentAction?: StreamDeckAction;

	/**
	 * Updates the key's image with a clock icon in the selected color
	 */
	private async updateKeyImage(settings: ClockSettings, action: StreamDeckAction): Promise<void> {
		const r = settings.red ?? 255;
		const g = settings.green ?? 0;
		const b = settings.blue ?? 0;
		
		// Create an SVG clock icon with the selected color
		const svg = `<svg width="144" height="144" viewBox="0 0 144 144">
			<circle cx="72" cy="72" r="60" fill="none" stroke="rgb(${r},${g},${b})" stroke-width="8"/>
			<line x1="72" y1="72" x2="72" y2="30" stroke="rgb(${r},${g},${b})" stroke-width="6" stroke-linecap="round"/>
			<line x1="72" y1="72" x2="100" y2="72" stroke="rgb(${r},${g},${b})" stroke-width="6" stroke-linecap="round"/>
		</svg>`;

		await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
	}

	/**
	 * Called when the action appears on the Stream Deck
	 */
	override async onWillAppear(ev: WillAppearEvent<ClockSettings>): Promise<void> {
		const localSettings = ev.payload.settings;
		this.currentAction = ev.action;
		await this.updateKeyImage(localSettings, ev.action);
	}

	/**
	 * Called when the action key is pressed
	 */
	override async onKeyDown(ev: KeyDownEvent<ClockSettings>): Promise<void> {
		const settings = ev.payload.settings;
		
		// Validate IP address
		if (!settings.espIP) {
			await ev.action.showAlert();
			streamDeck.logger.error('ESP8266 IP address not configured');
			return;
		}

		// Set default values if not configured
		const r = settings.red ?? 255;
		const g = settings.green ?? 0;
		const b = settings.blue ?? 0;

		try {
			// Make HTTP request to ESP8266
			const response = await fetch(`http://${settings.espIP}/color?r=${r}&g=${g}&b=${b}`);
			//const response = await fetch(`http://192.168.1.145/color?r=255&g=255&b=255`);
			
			if (!response.ok) {
				throw new Error(`HTTP error! status: ${response.status}`);
			}
			
			await ev.action.showOk();
		} catch (error) {
			await ev.action.showAlert();
			streamDeck.logger.error(`Failed to send color to ESP8266: ${error}`);
		}
	}

	/**
	 * Called when settings are updated
	 */
	override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<ClockSettings>): Promise<void> {
		// if setting espIP is not set, use global settings or if espIp is changed change in global settings
		const globalSettings = await streamDeck.settings.getGlobalSettings<GlobalSettings>();
		if (!ev.payload.settings.espIP) {
			ev.payload.settings.espIP = globalSettings.espIP;
		}
		if (ev.payload.settings.espIP !== globalSettings.espIP) {
			await streamDeck.settings.setGlobalSettings({ espIP: ev.payload.settings.espIP });
		}
		await this.updateKeyImage(ev.payload.settings, ev.action);
	}
}

/**
 * Settings for ClockColorControl
 */
type ClockSettings = {
	espIP?: string;
	red?: number;
	green?: number;
	blue?: number;
};
