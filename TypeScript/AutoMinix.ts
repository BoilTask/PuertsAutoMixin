import * as UE from "ue";
import { blueprint } from "puerts";

export async function ToMinix(c: UE.Class, m: string) {
    let modulePath = m;
    let exportName: string | null = null;
    if (m.includes(":")) {
        const parts = m.split(":");
        modulePath = parts[0];
        exportName = parts[1];
    }
    const mod = await import(modulePath);
    const TargetClass = exportName ? mod[exportName] : mod.default;
    if (!TargetClass) {
        throw new Error(`not found: ${m}`);
    }
    blueprint.mixin(blueprint.tojs(c), TargetClass);
}
