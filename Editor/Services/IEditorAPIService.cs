namespace FirstEngineEditor.Services
{
    /// <summary>
    /// EditorAPI服务接口，用于与C++引擎通信
    /// </summary>
    public interface IEditorAPIService
    {
        /// <summary>
        /// 同步Entity属性到引擎
        /// </summary>
        /// <param name="entityId">Entity ID</param>
        /// <returns>是否成功</returns>
        bool SyncEntityToEngine(int entityId);

        /// <summary>
        /// 更新Entity的Transform到引擎
        /// </summary>
        /// <param name="entityId">Entity ID</param>
        /// <param name="position">位置</param>
        /// <param name="rotation">旋转</param>
        /// <param name="scale">缩放</param>
        /// <returns>是否成功</returns>
        bool UpdateEntityTransform(int entityId, float[]? position, float[]? rotation, float[]? scale);

        /// <summary>
        /// 更新Component属性到引擎
        /// </summary>
        /// <param name="entityId">Entity ID</param>
        /// <param name="componentType">Component类型</param>
        /// <param name="propertyName">属性名</param>
        /// <param name="propertyValue">属性值</param>
        /// <returns>是否成功</returns>
        bool UpdateComponentProperty(int entityId, int componentType, string propertyName, object? propertyValue);
    }
}
