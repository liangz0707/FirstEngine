using System;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;
using FirstEngineEditor.ViewModels.Panels;
using FirstEngineEditor.Services;

namespace FirstEngineEditor.Views.Panels
{
    public partial class PropertyPanelView : System.Windows.Controls.UserControl
    {
        private PropertyPanelViewModel? _viewModel;

        public PropertyPanelView()
        {
            InitializeComponent();
            Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            _viewModel = DataContext as PropertyPanelViewModel;
        }

        private void OnPropertyValueChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender is System.Windows.Controls.TextBox textBox && textBox.Tag is PropertyItem propertyItem && _viewModel != null)
                {
                    _viewModel.UpdatePropertyValue(propertyItem, textBox.Text);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnPropertyValueChanged: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
            }
        }

        private void OnRemoveComponent(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender is System.Windows.Controls.Button button && button.Tag is ComponentItem componentItem && _viewModel != null)
                {
                    _viewModel.RemoveComponent(componentItem);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnRemoveComponent: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error removing component: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnComponentsDragOver(object sender, System.Windows.DragEventArgs e)
        {
            // Check if we have a selected entity
            if (_viewModel == null || _viewModel.SelectedEntity == null)
            {
                e.Effects = System.Windows.DragDropEffects.None;
                e.Handled = false;
                return;
            }

            // Allow dropping Model resources from resource browser
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                try
                {
                    var resourceItem = e.Data.GetData("ResourceBrowserItem");
                    System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Got ResourceBrowserItem, type={resourceItem?.GetType().Name}");
                    
                    if (resourceItem != null)
                    {
                        string? typeValue = null;
                        int? idValue = null;
                        
                        // First try direct cast
                        var item = resourceItem as ResourceBrowserItem;
                        if (item != null)
                        {
                            typeValue = item.Type;
                            idValue = item.Id;
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Direct cast - Type={typeValue}, Id={idValue}");
                        }
                        else
                        {
                            // Try reflection as fallback
                            var typeProperty = resourceItem.GetType().GetProperty("Type");
                            var idProperty = resourceItem.GetType().GetProperty("Id");
                            
                            if (typeProperty != null)
                            {
                                typeValue = typeProperty.GetValue(resourceItem)?.ToString();
                            }
                            if (idProperty != null)
                            {
                                var idObj = idProperty.GetValue(resourceItem);
                                if (idObj != null && int.TryParse(idObj.ToString(), out int id))
                                {
                                    idValue = id;
                                }
                            }
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Reflection - Type={typeValue}, Id={idValue}");
                        }
                        
                        // Check if it's a Model resource (case-insensitive)
                        if (!string.IsNullOrEmpty(typeValue) && 
                            typeValue.Equals("Model", StringComparison.OrdinalIgnoreCase))
                        {
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Allowing Model drop (Type={typeValue}, Id={idValue})");
                            e.Effects = System.Windows.DragDropEffects.Link;
                            e.Handled = true;
                            return;
                        }
                        else
                        {
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Rejecting drop - Type={typeValue} is not Model");
                        }
                    }
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error in OnComponentsDragOver: {ex.Message}");
                    System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"OnComponentsDragOver: Data format 'ResourceBrowserItem' not present. Available formats: {string.Join(", ", e.Data.GetFormats())}");
            }
            
            e.Effects = System.Windows.DragDropEffects.None;
            e.Handled = false;
        }

        private void OnComponentsDrop(object sender, System.Windows.DragEventArgs e)
        {
            if (_viewModel == null || _viewModel.SelectedEntity == null)
            {
                System.Diagnostics.Debug.WriteLine("OnComponentsDrop: No view model or selected entity");
                e.Handled = false;
                return;
            }

            // Handle resource browser drop (Model)
            if (e.Data.GetDataPresent("ResourceBrowserItem"))
            {
                try
                {
                    var resourceItem = e.Data.GetData("ResourceBrowserItem");
                    System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Got ResourceBrowserItem, type={resourceItem?.GetType().Name}");
                    
                    if (resourceItem != null)
                    {
                        string? typeValue = null;
                        int? idValue = null;
                        
                        // First try direct cast
                        var item = resourceItem as ResourceBrowserItem;
                        if (item != null)
                        {
                            typeValue = item.Type;
                            idValue = item.Id;
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Direct cast - Type={typeValue}, Id={idValue}");
                        }
                        else
                        {
                            // Try reflection as fallback
                            var typeProperty = resourceItem.GetType().GetProperty("Type");
                            var idProperty = resourceItem.GetType().GetProperty("Id");
                            
                            if (typeProperty != null)
                            {
                                typeValue = typeProperty.GetValue(resourceItem)?.ToString();
                            }
                            if (idProperty != null)
                            {
                                var idObj = idProperty.GetValue(resourceItem);
                                if (idObj != null && int.TryParse(idObj.ToString(), out int id))
                                {
                                    idValue = id;
                                }
                            }
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Reflection - Type={typeValue}, Id={idValue}");
                        }
                        
                        // Check if it's a Model resource (case-insensitive) and has valid ID
                        if (!string.IsNullOrEmpty(typeValue) && 
                            typeValue.Equals("Model", StringComparison.OrdinalIgnoreCase) &&
                            idValue.HasValue && idValue.Value > 0)
                        {
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Adding Model component with ID={idValue.Value}");
                            // Add Model component (type 1)
                            _viewModel.AddComponentToEntity(1, idValue.Value);
                            e.Handled = true;
                            return;
                        }
                        else
                        {
                            System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Cannot add component - Type={typeValue}, Id={idValue}");
                        }
                    }
                }
                catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine($"Error in OnComponentsDrop: {ex.Message}");
                    System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                    System.Windows.MessageBox.Show($"Error adding component: {ex.Message}", "Error", 
                        System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
            }
            else
            {
                System.Diagnostics.Debug.WriteLine($"OnComponentsDrop: Data format 'ResourceBrowserItem' not present. Available formats: {string.Join(", ", e.Data.GetFormats())}");
            }
            
            System.Diagnostics.Debug.WriteLine("OnComponentsDrop: No valid drop data");
            e.Handled = false;
        }

        private void OnComponentsContextMenuOpening(object sender, System.Windows.Controls.ContextMenuEventArgs e)
        {
            // Context menu is defined in XAML, this handler can be used for additional logic if needed
        }

        private void OnAddCameraComponent(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_viewModel != null)
                {
                    _viewModel.AddCameraComponentToEntity();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnAddCameraComponent: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error adding camera component: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnSetMainCamera(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender is System.Windows.Controls.MenuItem menuItem && menuItem.Tag is ComponentItem componentItem && _viewModel != null)
                {
                    _viewModel.SetMainCamera(componentItem);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error in OnSetMainCamera: {ex.Message}");
                System.Diagnostics.Debug.WriteLine($"Stack trace: {ex.StackTrace}");
                System.Windows.MessageBox.Show($"Error setting main camera: {ex.Message}", "Error", 
                    System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }
    }
}
