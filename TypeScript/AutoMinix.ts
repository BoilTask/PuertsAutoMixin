import * as UE from "ue";
import { blueprint } from "puerts";

/**
 * 收集类及其父类原型链上的所有方法
 * @param targetClass 目标类
 * @returns 包含所有继承方法的代理类
 */
function collectInheritedMethods<T extends new (...args: any[]) => any>(targetClass: T): T {
    const methods: Record<string, Function> = {};
    let currentProto = targetClass.prototype;
    
    // 遍历原型链，收集所有方法
    while (currentProto && currentProto !== Object.prototype) {
        const ownProps = Object.getOwnPropertyNames(currentProto);
        for (const name of ownProps) {
            // 跳过 constructor 和已收集的方法（子类覆盖父类）
            if (name === 'constructor' || methods.hasOwnProperty(name)) {
                continue;
            }
            const descriptor = Object.getOwnPropertyDescriptor(currentProto, name);
            if (descriptor && typeof descriptor.value === 'function') {
                methods[name] = descriptor.value;
            }
        }
        currentProto = Object.getPrototypeOf(currentProto);
    }
    
    // 创建代理类，包含所有收集到的方法
    class InheritedClass {
        constructor(...args: any[]) {
            // 调用原始构造函数
            const result = new (targetClass as any)(...args);
            return result;
        }
    }
    
    // 将所有方法添加到代理类原型
    for (const [name, method] of Object.entries(methods)) {
        (InheritedClass.prototype as any)[name] = method;
    }
    
    return InheritedClass as unknown as T;
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
    
    // 收集继承的方法后再 mixin
    const classWithInheritance = collectInheritedMethods(TargetClass);
    blueprint.mixin(blueprint.tojs(c), classWithInheritance);
}
