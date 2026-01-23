using System;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Services
{
    /// <summary>
    /// EditorAPI服务实现，用于与C++引擎通信
    /// </summary>
    public class EditorAPIService : IEditorAPIService
    {
        private readonly RenderEngineService? _renderEngineService;

        public EditorAPIService()
        {
            // 尝试从ServiceLocator获取RenderEngineService
            _renderEngineService = ServiceLocator.Get<RenderEngineService>();
        }

        public bool SyncEntityToEngine(int entityId)
        {
            // TODO: 实现Entity同步到引擎的逻辑
            // 目前可以通过重新加载场景来实现，但效率较低
            // 更好的方式是扩展EditorAPI添加UpdateEntity方法
            System.Diagnostics.Debug.WriteLine($"SyncEntityToEngine: Entity {entityId} (not implemented yet)");
            return false;
        }

        public bool UpdateEntityTransform(int entityId, float[]? position, float[]? rotation, float[]? scale)
        {
            // TODO: 实现Transform更新到引擎的逻辑
            // 需要扩展EditorAPI添加UpdateEntityTransform方法
            System.Diagnostics.Debug.WriteLine($"UpdateEntityTransform: Entity {entityId} (not implemented yet)");
            return false;
        }

        public bool UpdateComponentProperty(int entityId, int componentType, string propertyName, object? propertyValue)
        {
            // TODO: 实现Component属性更新到引擎的逻辑
            // 需要扩展EditorAPI添加UpdateComponentProperty方法
            System.Diagnostics.Debug.WriteLine($"UpdateComponentProperty: Entity {entityId}, Component {componentType}, Property {propertyName} (not implemented yet)");
            return false;
        }
    }
}
