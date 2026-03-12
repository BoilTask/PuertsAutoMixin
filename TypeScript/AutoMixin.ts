import * as UE from "ue";
import { blueprint } from "puerts";

type Constructor = new (...args: any[]) => any;

function getInheritanceChain(cls: Constructor): Constructor[] {
    const chain: Constructor[] = [];
    const visited = new Set<Constructor>();
    let current: Constructor | null = cls;

    while (current && !visited.has(current)) {
        visited.add(current);
        if (current.prototype && typeof current.prototype === 'object') {
            chain.push(current);
        }
        const proto = Object.getPrototypeOf(current);
        if (proto === Function.prototype || proto === Object || !proto) {
            break;
        }
        current = proto;
    }

    return chain;
}

function collectMethods(chain: Constructor[]): { [key: string]: Function } {
    const methods: { [key: string]: Function } = {};
    for (let i = chain.length - 1; i >= 0; i--) {
        const cls = chain[i];
        const names = Object.getOwnPropertyNames(cls.prototype);
        for (const name of names) {
            if (name === 'constructor') continue;
            const descriptor = Object.getOwnPropertyDescriptor(cls.prototype, name);
            if (descriptor && typeof descriptor.value === 'function') {
                methods[name] = descriptor.value;
            }
        }
    }
    return methods;
}

export function ToMinix(c: UE.Class, m: string) {
    let modulePath = m;
    let exportName: string | null = null;
    if (m.includes(":")) {
        const parts = m.split(":");
        modulePath = parts[0];
        exportName = parts[1];
    }
    const mod = require(modulePath);
    const TargetClass = exportName ? mod[exportName] : mod.default;
    if (!TargetClass) {
        throw new Error(`not found: ${m}`);
    }

    const chain = getInheritanceChain(TargetClass);
    const methods = collectMethods(chain);

    const MergedClass = function() {} as unknown as Constructor;
    MergedClass.prototype = Object.create({});
    for (const name in methods) {
        MergedClass.prototype[name] = methods[name];
    }

    blueprint.mixin(blueprint.tojs(c), MergedClass);
}
