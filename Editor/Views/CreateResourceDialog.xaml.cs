using System;
using System.Collections.Generic;
using System.Windows;
using FirstEngineEditor.Services;

namespace FirstEngineEditor.Views
{
    public partial class CreateResourceDialog : Window
    {
        private IResourceImportService? _importService;

        public CreateResourceDialog()
        {
            InitializeComponent();
            _importService = ServiceLocator.Get<IResourceImportService>();
            
            ResourceTypeComboBox.SelectionChanged += OnResourceTypeChanged;
            UpdateUI();
        }

        private void OnResourceTypeChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            UpdateUI();
        }

        private void UpdateUI()
        {
            if (ResourceTypeComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem item)
            {
                string type = item.Content.ToString() ?? "";
                
                // Update virtual path hint
                if (string.IsNullOrEmpty(VirtualPathTextBox.Text) || 
                    VirtualPathTextBox.Text.StartsWith("materials/") || 
                    VirtualPathTextBox.Text.StartsWith("scenes/") ||
                    VirtualPathTextBox.Text.StartsWith("lights/"))
                {
                    string basePath = type.ToLower() + "s/";
                    if (type == "SceneLevel")
                        basePath = "scenes/";
                    VirtualPathTextBox.Text = basePath + "new_" + type.ToLower();
                }

                // Show/hide properties based on type
                if (type == "Material")
                {
                    PropertiesGroupBox.Visibility = Visibility.Visible;
                    ShaderNameTextBox.Visibility = Visibility.Visible;
                }
                else
                {
                    PropertiesGroupBox.Visibility = Visibility.Collapsed;
                }
            }
        }

        private async void OnCreate(object sender, RoutedEventArgs e)
        {
            if (_importService == null)
            {
                System.Windows.MessageBox.Show("Import service not available", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            string virtualPath = VirtualPathTextBox.Text;
            if (string.IsNullOrEmpty(virtualPath))
            {
                System.Windows.MessageBox.Show("Please enter a virtual path", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            ResourceType resourceType = GetSelectedResourceType();
            
            Dictionary<string, object>? properties = null;
            if (resourceType == ResourceType.Material)
            {
                properties = new Dictionary<string, object>
                {
                    { "Shader", ShaderNameTextBox.Text }
                };
            }

            bool success = await _importService.CreateResourceAsync(virtualPath, resourceType, properties);
            
            if (success)
            {
                DialogResult = true;
                Close();
            }
            else
            {
                System.Windows.MessageBox.Show("Failed to create resource", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void OnCancel(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }

        private ResourceType GetSelectedResourceType()
        {
            if (ResourceTypeComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem item)
            {
                string type = item.Content.ToString() ?? "Material";
                if (type == "SceneLevel")
                    return ResourceType.SceneLevel;
                return Enum.Parse<ResourceType>(type);
            }
            return ResourceType.Material;
        }
    }
}
