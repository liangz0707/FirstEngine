using System;

namespace FirstEngineEditor.Attributes
{
    /// <summary>
    /// 标记属性应该在属性面板中显示
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public class PropertyAttribute : Attribute
    {
        /// <summary>
        /// 显示名称（如果为空则使用属性名）
        /// </summary>
        public string? DisplayName { get; set; }

        /// <summary>
        /// 显示顺序（数字越小越靠前）
        /// </summary>
        public int Order { get; set; } = 100;

        /// <summary>
        /// 是否可编辑
        /// </summary>
        public bool Editable { get; set; } = true;

        /// <summary>
        /// 属性描述/提示
        /// </summary>
        public string? Description { get; set; }

        /// <summary>
        /// 属性分类（用于分组显示）
        /// </summary>
        public string? Category { get; set; }

        /// <summary>
        /// 是否同步到引擎
        /// </summary>
        public bool SyncToEngine { get; set; } = true;

        public PropertyAttribute()
        {
        }

        public PropertyAttribute(string displayName)
        {
            DisplayName = displayName;
        }
    }

    /// <summary>
    /// 标记属性不应该在属性面板中显示
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public class PropertyIgnoreAttribute : Attribute
    {
    }
}
