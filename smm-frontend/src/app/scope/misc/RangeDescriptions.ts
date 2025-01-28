export interface RangeDescription {
    name: string,
    unit: string,
    values: number[],
    defaultIndex: number,
}

export const RangeDescriptions = {
    VOLTAGE: {
        name: "Napięcie",
        unit: "V",
        values: [100, 200, 350, 420, 1024, 2048, 4096],
        defaultIndex: 1,
    } as RangeDescription,
    CURRENT: {
        name: "Prąd",
        unit: "A",
        values: [0.1, 0.5, 1, 2, 3, 5, 10, 16, 25, 100, 500, 1024, 2048, 4096],
        defaultIndex: 1,
    } as RangeDescription,
    POWER: {
        name: "Moc",
        unit: "W",
        values: [50, 100, 400, 800, 1500, 3000, 5500, 11000],
        defaultIndex: 1,
    } as RangeDescription,
    WINDOW_SPAN: {
        name: "Szerokość Okna",
        unit: "ms",
        values: [20, 60, 160, 240, 400, 800, 3000, 8000],
        defaultIndex: 1,
    } as RangeDescription,
} as const;

export function createFormatterForRangeDescription(
    description: RangeDescription
): ((v: number) => string) {
    return (v: number) => (v + "").substring(0, 5) + description.unit;
}
