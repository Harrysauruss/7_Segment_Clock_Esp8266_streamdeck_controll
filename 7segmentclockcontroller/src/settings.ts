export type GlobalSettings = {
    globalEspIP?: string;
};

export type ActionSettings = {
    espIP?: string;
};

/**
 * Helper to determine the target IP.
 * Prefers the action-specific override IP; falls back to the global IP.
 * Returns undefined if neither is set.
 */
export function getTargetIP(settings: ActionSettings, globalSettings: GlobalSettings): string | undefined {
    // If the action specifies an override that isn't empty, use it.
    if (settings.espIP && settings.espIP.trim() !== '') {
        return settings.espIP;
    }
    
    // Otherwise, fall back to the global IP.
    if (globalSettings.globalEspIP && globalSettings.globalEspIP.trim() !== '') {
        return globalSettings.globalEspIP;
    }

    return undefined;
}
