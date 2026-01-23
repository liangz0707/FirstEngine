using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using FirstEngineEditor.Attributes;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Utils
{
    /// <summary>
    /// 属性反射辅助类，用于自动发现和显示对象属性
    /// </summary>
    public static class PropertyReflectionHelper
    {
        /// <summary>
        /// 从对象中提取所有标记了PropertyAttribute的属性
        /// </summary>
        public static List<PropertyItem> ExtractProperties(object obj)
        {
            var properties = new List<PropertyItem>();

            if (obj == null)
                return properties;

            var type = obj.GetType();
            var allProperties = type.GetProperties(BindingFlags.Public | BindingFlags.Instance);

            foreach (var prop in allProperties)
            {
                // 检查是否被忽略
                if (prop.GetCustomAttribute<PropertyIgnoreAttribute>() != null)
                    continue;

                // 检查是否有PropertyAttribute
                var attr = prop.GetCustomAttribute<PropertyAttribute>();

                // 如果没有PropertyAttribute，默认显示所有公共属性
                // 或者根据配置决定是否显示未标记的属性
                if (attr == null)
                {
                    // 默认显示所有公共属性（可以根据需要修改这个行为）
                    attr = new PropertyAttribute { DisplayName = prop.Name, Order = 1000 };
                }

                // 获取属性值
                object? value = null;
                try
                {
                    value = prop.GetValue(obj);
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error getting property {prop.Name}: {ex.Message}");
                    continue;
                }

                // 格式化属性值
                string valueString = FormatPropertyValue(value, prop.PropertyType);

                // 创建PropertyItem
                var propertyItem = new PropertyItem
                {
                    Name = attr.DisplayName ?? prop.Name,
                    Value = valueString,
                    Type = GetTypeDisplayName(prop.PropertyType),
                    PropertyInfo = prop,
                    Object = obj,
                    Category = attr.Category,
                    Description = attr.Description,
                    Editable = attr.Editable,
                    SyncToEngine = attr.SyncToEngine,
                    Order = attr.Order
                };

                properties.Add(propertyItem);
            }

            // 按Order排序
            return properties.OrderBy(p => p.Order).ThenBy(p => p.Name).ToList();
        }

        /// <summary>
        /// 格式化属性值为字符串
        /// </summary>
        private static string FormatPropertyValue(object? value, Type propertyType)
        {
            if (value == null)
                return "null";

            // 处理数组类型（如float[]）
            if (propertyType.IsArray)
            {
                var array = value as Array;
                if (array != null)
                {
                    var elements = new List<string>();
                    foreach (var item in array)
                    {
                        elements.Add(item?.ToString() ?? "null");
                    }
                    return string.Join(", ", elements);
                }
            }

            // 处理可空类型
            if (propertyType.IsGenericType && propertyType.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                return value.ToString() ?? "null";
            }

            // 处理基本类型
            return value.ToString() ?? "null";
        }

        /// <summary>
        /// 获取类型的显示名称
        /// </summary>
        private static string GetTypeDisplayName(Type type)
        {
            // 处理数组类型
            if (type.IsArray)
            {
                var elementType = type.GetElementType();
                if (elementType != null)
                {
                    return $"{GetTypeDisplayName(elementType)}[]";
                }
            }

            // 处理可空类型
            if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
            {
                var underlyingType = Nullable.GetUnderlyingType(type);
                if (underlyingType != null)
                {
                    return $"{GetTypeDisplayName(underlyingType)}?";
                }
            }

            // 处理特殊类型
            if (type == typeof(float[]))
                return "Vector3";
            if (type == typeof(int[]))
                return "Int32[]";

            // 返回类型名称
            return type.Name;
        }

        /// <summary>
        /// 设置属性值
        /// </summary>
        public static bool SetPropertyValue(PropertyItem item, string newValue)
        {
            if (item.PropertyInfo == null || item.Object == null)
                return false;

            try
            {
                var propertyType = item.PropertyInfo.PropertyType;
                object? convertedValue = null;

                // 处理不同属性类型
                if (propertyType == typeof(string))
                {
                    convertedValue = newValue;
                }
                else if (propertyType == typeof(int))
                {
                    if (int.TryParse(newValue, out int intValue))
                        convertedValue = intValue;
                    else
                        return false;
                }
                else if (propertyType == typeof(int?))
                {
                    if (string.IsNullOrEmpty(newValue) || newValue == "null")
                        convertedValue = null;
                    else if (int.TryParse(newValue, out int intValue))
                        convertedValue = intValue;
                    else
                        return false;
                }
                else if (propertyType == typeof(bool))
                {
                    if (bool.TryParse(newValue, out bool boolValue))
                        convertedValue = boolValue;
                    else
                        return false;
                }
                else if (propertyType == typeof(float))
                {
                    if (float.TryParse(newValue, out float floatValue))
                        convertedValue = floatValue;
                    else
                        return false;
                }
                else if (propertyType == typeof(float?))
                {
                    if (string.IsNullOrEmpty(newValue) || newValue == "null")
                        convertedValue = null;
                    else if (float.TryParse(newValue, out float floatValue))
                        convertedValue = floatValue;
                    else
                        return false;
                }
                else if (propertyType == typeof(float[]))
                {
                    // 解析Vector3格式: "x, y, z"
                    var parts = newValue.Split(',');
                    if (parts.Length == 3)
                    {
                        var floats = new float[3];
                        bool valid = true;
                        for (int i = 0; i < 3; i++)
                        {
                            if (float.TryParse(parts[i].Trim(), out float f))
                            {
                                floats[i] = f;
                            }
                            else
                            {
                                valid = false;
                                break;
                            }
                        }
                        if (valid)
                            convertedValue = floats;
                        else
                            return false;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    // 尝试使用TypeConverter或其他方式转换
                    System.Diagnostics.Debug.WriteLine($"Unsupported property type: {propertyType.Name}");
                    return false;
                }

                // 设置属性值
                item.PropertyInfo.SetValue(item.Object, convertedValue);
                item.Value = newValue;

                return true;
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error setting property {item.Name}: {ex.Message}");
                return false;
            }
        }
    }
}
