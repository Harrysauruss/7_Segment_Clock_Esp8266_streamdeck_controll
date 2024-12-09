import { action, KeyDownEvent, SingletonAction, WillAppearEvent, DidReceiveSettingsEvent, KeyAction } from "@elgato/streamdeck";

type ClockSettings = {
    ipAddress?: string;
    brightness?: number;
    color?: {
        r: number;
        g: number;
        b: number;
    };
};

type ClockStatus = {
    configured: boolean;
    brightness: number;
    color: {
        r: number;
        g: number;
        b: number;
    };
    currentHour: number;
    currentMinute: number;
    isTransitioning: boolean;
};

@action({ UUID: "com.marius.7-segment-esp-clock-controller.clock-control" })
export class ClockController extends SingletonAction<ClockSettings> {
    private status: ClockStatus | null = null;
    private updateInterval: NodeJS.Timeout | null = null;

    override async onWillAppear(ev: WillAppearEvent<ClockSettings>): Promise<void> {
        if (!('keyIndex' in ev.action)) return; // Only handle key actions
        // Start periodic status updates
        this.updateInterval = setInterval(() => this.updateStatus(ev.action as KeyAction<ClockSettings>), 5000);
        await this.updateStatus(ev.action as KeyAction<ClockSettings>);
    }

    override onWillDisappear(): void {
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }
    }

    override async onKeyDown(ev: KeyDownEvent<ClockSettings>): Promise<void> {
        const { ipAddress, brightness, color } = ev.payload.settings;
        
        if (!ipAddress) {
            await ev.action.showAlert();
            return;
        }

        try {
            const response = await fetch(`http://${ipAddress}/api/settings`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    brightness: brightness ?? 255,
                    color: color ?? { r: 255, g: 0, b: 0 }
                })
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            await ev.action.showOk();
            await this.updateStatus(ev.action);
        } catch (error) {
            console.error('Failed to update settings:', error);
            await ev.action.showAlert();
        }
    }

    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<ClockSettings>): Promise<void> {
        if (!('keyIndex' in ev.action)) return; // Only handle key actions
        await this.updateStatus(ev.action as KeyAction<ClockSettings>);
    }

    private async updateStatus(action: KeyAction<ClockSettings>): Promise<void> {
        const settings = await action.getSettings();
        const { ipAddress } = settings;
        
        if (!ipAddress) {
            await action.setTitle("No IP\nSet");
            return;
        }

        try {
            const response = await fetch(`http://${ipAddress}/api/status`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            this.status = await response.json() as ClockStatus;
            
            // Update the key's display
            const { brightness, color } = this.status;
            await action.setTitle(
                `RGB: ${color.r},${color.g},${color.b}\nBright: ${brightness}`
            );

            // Create an SVG preview of the current color
            const svg = `<svg width="72" height="72">
                <rect width="72" height="72" fill="rgb(${color.r},${color.g},${color.b})" />
            </svg>`;
            
            await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
        } catch (error) {
            console.error('Failed to fetch status:', error);
            await action.setTitle("Error\nCheck IP");
            await action.setImage(undefined); // Reset to default image
        }
    }
} 