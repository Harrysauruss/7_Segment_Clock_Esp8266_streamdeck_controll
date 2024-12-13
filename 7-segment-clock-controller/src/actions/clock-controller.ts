import { action, KeyDownEvent, SingletonAction, WillAppearEvent, DidReceiveSettingsEvent } from "@elgato/streamdeck";

type ClockSettings = {
    ipAddress?: string;
    color?: {
        r: number;
        g: number;
        b: number;
    };
};

@action({ UUID: "com.marius.7-segment-esp-clock-controller.clock-control" })
export class ClockController extends SingletonAction<ClockSettings> {
    override async onWillAppear(ev: WillAppearEvent<ClockSettings>): Promise<void> {
        await this.updatePreview(ev.payload.settings);
    }

    override async onKeyDown(ev: KeyDownEvent<ClockSettings>): Promise<void> {
        const { ipAddress, color = { r: 255, g: 0, b: 0 } } = ev.payload.settings;
        
        console.log('Settings:', { ipAddress, color });
        
        if (!ipAddress) {
            console.log('No IP address configured');
            await ev.action.showAlert();
            return;
        }

        try {
            const url = `http://${ipAddress}/color?r=${color.r}&g=${color.g}&b=${color.b}`;
            console.log('Calling URL:', url);

            const response = await fetch(url, {
                method: 'GET',
                signal: AbortSignal.timeout(5000)
            });
            
            console.log('Response status:', response.status);
            console.log('Response headers:', Object.fromEntries(response.headers));
            
            try {
                const text = await response.text();
                console.log('Response body:', text);
            } catch (e) {
                console.log('No response body or error reading it:', e);
            }

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            await ev.action.showOk();
            await this.updatePreview(ev.payload.settings);
        } catch (error) {
            console.error('Failed to update color:', error);
            if (error instanceof Error) {
                console.error('Error details:', {
                    name: error.name,
                    message: error.message,
                    stack: error.stack,
                    cause: error.cause
                });
            }
            await ev.action.showAlert();
        }
    }

    override async onDidReceiveSettings(ev: DidReceiveSettingsEvent<ClockSettings>): Promise<void> {
        await this.updatePreview(ev.payload.settings);
    }

    private async updatePreview(settings: ClockSettings): Promise<void> {
        const { ipAddress, color = { r: 255, g: 0, b: 0 } } = settings;
        
        // Get the first action from the enumerable
        const action = Array.from(this.actions)[0];
        if (!action) return;
        
        if (!ipAddress) {
            await action.setTitle("No IP\nSet");
            return;
        }

        // Update the key's display
        await action.setTitle(
            `RGB:\n${color.r},${color.g},${color.b}`
        );

        // Create an SVG preview of the current color
        const svg = `<svg width="72" height="72">
            <rect width="72" height="72" fill="rgb(${color.r},${color.g},${color.b})" />
        </svg>`;
        
        await action.setImage(`data:image/svg+xml,${encodeURIComponent(svg)}`);
    }
} 