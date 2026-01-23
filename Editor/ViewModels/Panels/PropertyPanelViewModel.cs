using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reflection;
using System.Windows.Threading;
using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels;
using FirstEngineEditor.ViewModels.Panels;
using FirstEngineEditor.Models;
using FirstEngineEditor.Utils;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class PropertyPanelViewModel : ViewModelBase
    {
        private object? _selectedObject;
        private ObservableCollection<PropertyItem> _properties = new();
        private ObservableCollection<ComponentItem> _components = new();
        private EntityData? _selectedEntity;

        public PropertyPanelViewModel()
        {
            Properties = new ObservableCollection<PropertyItem>();
            Components = new ObservableCollection<ComponentItem>();
        }

        public object? SelectedObject
        {
            get => _selectedObject;
            set
            {
                // Ensure we're on UI thread when updating properties
                var dispatcher = System.Windows.Application.Current?.Dispatcher;
                if (dispatcher != null && !dispatcher.CheckAccess())
                {
                    dispatcher.Invoke(() =>
                    {
                        SetProperty(ref _selectedObject, value);
                        UpdateProperties();
                    });
                }
                else
            {
                SetProperty(ref _selectedObject, value);
                UpdateProperties();
                }
            }
        }

        public ObservableCollection<PropertyItem> Properties { get; }
        public ObservableCollection<ComponentItem> Components { get; }

        public EntityData? SelectedEntity
        {
            get => _selectedEntity;
            private set => SetProperty(ref _selectedEntity, value);
        }

        private void UpdateProperties()
        {
            Properties.Clear();
            Components.Clear();
            SelectedEntity = null;

            if (_selectedObject == null) return;

            // Handle EntityData specially
            if (_selectedObject is EntityData entityData)
            {
                SelectedEntity = entityData;
                UpdateEntityProperties(entityData);
                return;
            }

            // Use reflection helper to automatically discover properties
            var discoveredProperties = PropertyReflectionHelper.ExtractProperties(_selectedObject);
            foreach (var prop in discoveredProperties)
            {
                Properties.Add(prop);
            }
        }

        private void UpdateEntityProperties(EntityData entityData)
        {
            try
            {
                // Use reflection helper to automatically discover Entity properties
                var entityProperties = PropertyReflectionHelper.ExtractProperties(entityData);
                foreach (var prop in entityProperties)
                {
                    Properties.Add(prop);
                }

                // Handle Transform properties separately (nested object)
                if (entityData.Transform != null)
                {
                    var transformProperties = PropertyReflectionHelper.ExtractProperties(entityData.Transform);
                    foreach (var prop in transformProperties)
                    {
                        Properties.Add(prop);
                    }
                }
                else
                {
                    // Create default transform if null
                    entityData.Transform = new TransformData
                    {
                        Position = new float[] { 0, 0, 0 },
                        Rotation = new float[] { 0, 0, 0 },
                        Scale = new float[] { 1, 1, 1 }
                    };
                    
                    var transformProperties = PropertyReflectionHelper.ExtractProperties(entityData.Transform);
                    foreach (var prop in transformProperties)
                    {
                        Properties.Add(prop);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error updating entity properties: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
            }

            // Components
            if (entityData.Components != null)
            {
                foreach (var component in entityData.Components)
                {
                    if (component != null)
                    {
                        var componentItem = new ComponentItem
                        {
                            ComponentData = component,
                            EntityData = entityData,
                            Type = GetComponentTypeName(component.Type),
                            ModelId = component.ModelID
                        };
                        
                        // Add additional info for Camera components
                        if (component.Type == 3) // Camera
                        {
                            if (component.IsMainCamera)
                            {
                                componentItem.Type = "Camera (Main)";
                            }
                            
                            var info = new List<string>();
                            if (component.FOV.HasValue) 
                                info.Add($"FOV: {component.FOV.Value:F1}°");
                            else
                                info.Add($"FOV: 60.0°");
                                
                            if (component.Near.HasValue) 
                                info.Add($"Near: {component.Near.Value:F2}");
                            else
                                info.Add($"Near: 0.10");
                                
                            if (component.Far.HasValue) 
                                info.Add($"Far: {component.Far.Value:F0}");
                            else
                                info.Add($"Far: 1000");
                                
                            componentItem.AdditionalInfo = string.Join(", ", info);
                        }
                        
                        Components.Add(componentItem);
                    }
                }
            }
        }

        private string GetComponentTypeName(int type)
        {
            return type switch
            {
                1 => "Model",
                2 => "Light",
                3 => "Camera",
                _ => $"Type_{type}"
            };
        }

        public void UpdatePropertyValue(PropertyItem item, string newValue)
        {
            if (item.PropertyInfo == null || item.Object == null)
                return;

            try
            {
                // Use reflection helper to set property value
                bool success = PropertyReflectionHelper.SetPropertyValue(item, newValue);
                
                if (success)
                {
                    // Notify scene to save
                    OnPropertyChanged(nameof(Properties));
                    
                    // Sync to engine if needed
                    if (item.SyncToEngine)
                    {
                        SyncPropertyToEngine(item);
                    }
                    
                    // Trigger scene save
                    SaveSceneIfNeeded();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error updating property: {ex.Message}");
            }
        }

        /// <summary>
        /// 同步属性修改到引擎
        /// </summary>
        private void SyncPropertyToEngine(PropertyItem item)
        {
            try
            {
                // 如果修改的是Entity的属性
                if (item.Object is EntityData entityData)
                {
                    SyncEntityToEngine(entityData);
                }
                // 如果修改的是Transform的属性
                else if (item.Object is TransformData transformData)
                {
                    // 找到对应的Entity
                    if (SelectedEntity != null)
                    {
                        SyncEntityToEngine(SelectedEntity);
                    }
                }
                // 如果修改的是Component的属性
                else if (item.Object is ComponentData componentData)
                {
                    if (SelectedEntity != null)
                    {
                        SyncEntityToEngine(SelectedEntity);
                    }
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error syncing property to engine: {ex.Message}");
            }
        }

        /// <summary>
        /// 同步Entity到引擎
        /// </summary>
        private void SyncEntityToEngine(EntityData entityData)
        {
            try
            {
                // 获取EditorAPI服务（可选）
                var editorAPIService = ServiceLocator.Get<IEditorAPIService>();
                if (editorAPIService != null)
                {
                    // 尝试同步Entity到引擎
                    editorAPIService.SyncEntityToEngine(entityData.Id);
                }
                else
                {
                    // 如果没有EditorAPIService，记录日志但不报错
                    // 属性修改仍然会保存到JSON文件中
                    System.Diagnostics.Debug.WriteLine($"EditorAPIService not available - property changes will be saved to JSON only");
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error syncing entity to engine: {ex.Message}");
                // 不抛出异常，允许属性修改继续保存到JSON
            }
        }

        private void SaveSceneIfNeeded()
        {
            if (SelectedEntity == null)
                return;

            // Get SceneResourceListViewModel and call SaveSceneData directly
            // This ensures we save the C# side JSON data, not the C++ engine state
            try
            {
                var mainViewModel = System.Windows.Application.Current?.MainWindow?.DataContext as MainViewModel;
                if (mainViewModel?.SceneResourceListViewModel != null)
                {
                    mainViewModel.SceneResourceListViewModel.SaveSceneData();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error saving scene: {ex.Message}");
            }
        }

        public void AddComponentToEntity(int componentType, int? modelId = null)
        {
            if (SelectedEntity == null)
            {
                System.Diagnostics.Debug.WriteLine("AddComponentToEntity: SelectedEntity is null");
                return;
            }

            // Ensure we're on UI thread
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
            {
                dispatcher.Invoke(() => AddComponentToEntityInternal(componentType, modelId));
            }
            else
            {
                AddComponentToEntityInternal(componentType, modelId);
            }
        }

        private void AddComponentToEntityInternal(int componentType, int? modelId = null)
        {
            if (SelectedEntity == null)
                return;

            if (SelectedEntity.Components == null)
                SelectedEntity.Components = new List<ComponentData>();

            var component = new ComponentData
            {
                Type = componentType,
                ModelID = modelId
            };

            // Initialize Camera component with default values
            if (componentType == 3) // Camera
            {
                component.FOV = 60.0f;
                component.Near = 0.1f;
                component.Far = 1000.0f;
                component.IsMainCamera = false;
                
                // If this is the first camera, make it the main camera
                if (SelectedEntity.Components.Count == 0 || 
                    !SelectedEntity.Components.Any(c => c != null && c.Type == 3 && c.IsMainCamera))
                {
                    component.IsMainCamera = true;
                }
            }

            SelectedEntity.Components.Add(component);

            var componentItem = new ComponentItem
            {
                ComponentData = component,
                EntityData = SelectedEntity,
                Type = GetComponentTypeName(componentType),
                ModelId = modelId
            };

            // Add additional info for Camera components
            if (componentType == 3) // Camera
            {
                if (component.IsMainCamera)
                {
                    componentItem.Type = "Camera (Main)";
                }
                
                var info = new List<string>();
                if (component.FOV.HasValue) info.Add($"FOV: {component.FOV.Value:F1}°");
                if (component.Near.HasValue) info.Add($"Near: {component.Near.Value:F2}");
                if (component.Far.HasValue) info.Add($"Far: {component.Far.Value:F0}");
                componentItem.AdditionalInfo = string.Join(", ", info);
            }

            Components.Add(componentItem);

            // Notify scene to save
            OnPropertyChanged(nameof(Components));
            SaveSceneIfNeeded();
        }

        public void AddCameraComponentToEntity()
        {
            if (SelectedEntity == null)
            {
                System.Diagnostics.Debug.WriteLine("AddCameraComponentToEntity: No selected entity");
                System.Windows.MessageBox.Show("请先选择一个Entity", "提示", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
                return;
            }
            AddComponentToEntity(3, null); // Type 3 = Camera
        }

        public void RemoveComponent(ComponentItem componentItem)
        {
            if (componentItem.EntityData?.Components != null)
            {
                var wasMainCamera = componentItem.ComponentData.Type == 3 && componentItem.ComponentData.IsMainCamera;
                
                componentItem.EntityData.Components.Remove(componentItem.ComponentData);
                Components.Remove(componentItem);
                
                // If we removed the main camera, make the first remaining camera the main camera
                if (wasMainCamera && componentItem.EntityData.Components != null)
                {
                    var firstCamera = componentItem.EntityData.Components.FirstOrDefault(c => c.Type == 3);
                    if (firstCamera != null)
                    {
                        firstCamera.IsMainCamera = true;
                        // Update the display
                        var cameraItem = Components.FirstOrDefault(c => c.ComponentData == firstCamera);
                        if (cameraItem != null)
                        {
                            cameraItem.Type = "Camera (Main)";
                        }
                    }
                }
                
                OnPropertyChanged(nameof(Components));
                SaveSceneIfNeeded();
            }
        }

        public void SetMainCamera(ComponentItem cameraItem)
        {
            if (cameraItem == null || cameraItem.ComponentData == null || cameraItem.ComponentData.Type != 3 || SelectedEntity == null)
            {
                System.Diagnostics.Debug.WriteLine("SetMainCamera: Invalid parameters");
                return;
            }

            // Ensure we're on UI thread
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
            {
                dispatcher.Invoke(() => SetMainCameraInternal(cameraItem));
            }
            else
            {
                SetMainCameraInternal(cameraItem);
            }
        }

        private void SetMainCameraInternal(ComponentItem cameraItem)
        {
            if (cameraItem == null || cameraItem.ComponentData == null || cameraItem.ComponentData.Type != 3 || SelectedEntity == null)
                return;

            // Unset all other cameras as main
            if (SelectedEntity.Components != null)
            {
                foreach (var component in SelectedEntity.Components)
                {
                    if (component != null && component.Type == 3)
                    {
                        component.IsMainCamera = false;
                    }
                }
            }

            // Set this camera as main
            cameraItem.ComponentData.IsMainCamera = true;
            
            // Update display for all camera components
            foreach (var item in Components)
            {
                if (item.ComponentData != null && item.ComponentData.Type == 3)
                {
                    if (item.ComponentData.IsMainCamera)
                    {
                        item.Type = "Camera (Main)";
                    }
                    else
                    {
                        item.Type = "Camera";
                    }
                }
            }
            
            OnPropertyChanged(nameof(Components));
            SaveSceneIfNeeded();
        }
    }

    public class PropertyItem
    {
        public string Name { get; set; } = string.Empty;
        public string Value { get; set; } = string.Empty;
        public string Type { get; set; } = string.Empty;
        public PropertyInfo? PropertyInfo { get; set; }
        public object? Object { get; set; }
        public string? Category { get; set; }
        public string? Description { get; set; }
        public bool Editable { get; set; } = true;
        public bool SyncToEngine { get; set; } = true;
        public int Order { get; set; } = 100;
    }

    public class ComponentItem
    {
        public ComponentData ComponentData { get; set; } = null!;
        public EntityData? EntityData { get; set; }
        public string Type { get; set; } = string.Empty;
        public int? ModelId { get; set; }
        public string? AdditionalInfo { get; set; }
    }
}
