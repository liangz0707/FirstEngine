# 属性反射系统使用说明

## 概述

属性反射系统允许通过反射自动发现和显示Entity和Component的属性，无需手动编写代码。系统使用特性（Attribute）来标记需要显示的属性，并支持自动同步到引擎。

## 主要功能

1. **自动属性发现**：通过反射自动发现标记了`PropertyAttribute`的属性
2. **属性标记**：使用`PropertyAttribute`特性标记需要显示的属性
3. **属性同步**：属性修改后自动同步到引擎（需要实现EditorAPI）
4. **类型支持**：支持基本类型、可空类型、数组类型等

## 使用方法

### 1. 标记属性

在EntityData、ComponentData或其他数据类中，使用`PropertyAttribute`标记需要显示的属性：

```csharp
public class EntityData
{
    [JsonProperty("id")]
    [Property(DisplayName = "ID", Order = 1, Editable = false, Category = "Basic")]
    public int Id { get; set; }

    [JsonProperty("name")]
    [Property(DisplayName = "Name", Order = 2, Category = "Basic")]
    public string? Name { get; set; }

    [JsonProperty("active")]
    [Property(DisplayName = "Active", Order = 3, Category = "Basic")]
    public bool Active { get; set; }

    // 使用PropertyIgnore标记不需要显示的属性
    [JsonProperty("components")]
    [PropertyIgnore]
    public List<ComponentData>? Components { get; set; }
}
```

### 2. PropertyAttribute参数说明

- `DisplayName`：显示名称（如果为空则使用属性名）
- `Order`：显示顺序（数字越小越靠前，默认100）
- `Editable`：是否可编辑（默认true）
- `Description`：属性描述/提示
- `Category`：属性分类（用于分组显示）
- `SyncToEngine`：是否同步到引擎（默认true）

### 3. 自动属性发现

系统会自动使用`PropertyReflectionHelper.ExtractProperties`方法发现属性：

```csharp
// 在PropertyPanelViewModel中自动调用
var properties = PropertyReflectionHelper.ExtractProperties(entityData);
```

### 4. 属性修改

属性修改会自动调用`PropertyReflectionHelper.SetPropertyValue`方法，支持以下类型：

- `string`
- `int`, `int?`
- `bool`
- `float`, `float?`
- `float[]` (Vector3格式: "x, y, z")

### 5. 引擎同步

属性修改后，如果`SyncToEngine`为true，会自动调用`SyncEntityToEngine`方法。目前需要实现`IEditorAPIService`接口来提供引擎同步功能。

## 扩展EditorAPI

要实现完整的引擎同步，需要：

1. 在C++端扩展EditorAPI，添加以下方法：
   - `EditorAPI_UpdateEntityTransform(int entityId, float* position, float* rotation, float* scale)`
   - `EditorAPI_UpdateComponentProperty(int entityId, int componentType, const char* propertyName, void* propertyValue)`

2. 在C#端实现`IEditorAPIService`接口的具体逻辑

3. 在`ServiceLocator`中注册`EditorAPIService`（可选）

## 示例

### 添加新属性到ComponentData

```csharp
public class ComponentData
{
    // 现有属性...

    [JsonProperty("newProperty")]
    [Property(DisplayName = "New Property", Order = 20, Category = "Custom", Description = "A new property")]
    public float? NewProperty { get; set; }
}
```

添加后，属性面板会自动显示这个新属性，无需修改其他代码。

### 隐藏属性

```csharp
[JsonProperty("internalData")]
[PropertyIgnore]
public object? InternalData { get; set; }
```

## 注意事项

1. 所有需要显示的属性都应该标记`PropertyAttribute`
2. 不需要显示的属性应该标记`PropertyIgnore`
3. 属性修改会先保存到JSON文件，然后尝试同步到引擎
4. 如果引擎同步失败，属性修改仍然会保存到JSON文件
5. 目前引擎同步功能需要扩展EditorAPI才能完全实现
