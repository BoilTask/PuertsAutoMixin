#pragma once

enum EPuertsBindingStatus
{
	// 未编译，未知
	Unknown,

	// 未绑定
	NotBound,

	// 已绑定
	Bound,

	// 已绑定，但没有找到对应的模块
	BoundButInvalid
};

/* 获取指定蓝图对象上的Puerts绑定状态 */
EPuertsBindingStatus GetBindingStatus(const UBlueprint* Blueprint);
FString GetScriptRealPath(FString Module);
