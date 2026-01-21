using System;
using System.IO;
using System.Windows;
using FirstEngineEditor.Services;

namespace FirstEngineEditor.Views
{
    public partial class ResourceImportDialog : Window
    {
        private IResourceImportService? _importService;

        public ResourceImportDialog()
        {
            InitializeComponent();
            _importService = ServiceLocator.Get<IResourceImportService>();
        }

        private void OnBrowseSourceFile(object sender, RoutedEventArgs e)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "All Supported|*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr;*.fbx;*.obj;*.dae;*.3ds;*.hlsl;*.glsl;*.vert;*.frag|" +
                         "Textures|*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr|" +
                         "Models|*.fbx;*.obj;*.dae;*.3ds|" +
                         "Shaders|*.hlsl;*.glsl;*.vert;*.frag",
                Title = "Select Resource File"
            };

            if (dialog.ShowDialog() == true)
            {
                SourceFileTextBox.Text = dialog.FileName;
                UpdateFileInfo(dialog.FileName);
                AutoDetectResourceType(dialog.FileName);
            }
        }

        private void UpdateFileInfo(string filePath)
        {
            if (!File.Exists(filePath))
            {
                FileNameTextBlock.Text = "No file selected";
                FileSizeTextBlock.Text = "";
                FileTypeTextBlock.Text = "";
                return;
            }

            var fileInfo = new FileInfo(filePath);
            FileNameTextBlock.Text = $"File: {fileInfo.Name}";
            FileSizeTextBlock.Text = $"Size: {FormatFileSize(fileInfo.Length)}";
            FileTypeTextBlock.Text = $"Type: {fileInfo.Extension.ToUpper()}";
        }

        private void AutoDetectResourceType(string filePath)
        {
            if (_importService == null) return;

            string ext = Path.GetExtension(filePath).ToLower();
            
            // Auto-detect type
            if (_importService.ValidateImportFile(filePath, ResourceType.Texture))
            {
                ResourceTypeComboBox.SelectedIndex = 0; // Texture
                if (string.IsNullOrEmpty(VirtualPathTextBox.Text) || VirtualPathTextBox.Text == "textures/")
                {
                    string name = Path.GetFileNameWithoutExtension(filePath);
                    VirtualPathTextBox.Text = $"textures/{name.ToLower()}";
                }
            }
            else if (_importService.ValidateImportFile(filePath, ResourceType.Model))
            {
                ResourceTypeComboBox.SelectedIndex = 1; // Model
                if (string.IsNullOrEmpty(VirtualPathTextBox.Text) || VirtualPathTextBox.Text == "textures/")
                {
                    string name = Path.GetFileNameWithoutExtension(filePath);
                    VirtualPathTextBox.Text = $"models/{name.ToLower()}";
                }
            }
            else if (_importService.ValidateImportFile(filePath, ResourceType.Shader))
            {
                ResourceTypeComboBox.SelectedIndex = 3; // Shader
                if (string.IsNullOrEmpty(VirtualPathTextBox.Text) || VirtualPathTextBox.Text == "textures/")
                {
                    string name = Path.GetFileNameWithoutExtension(filePath);
                    VirtualPathTextBox.Text = $"shaders/{name.ToLower()}";
                }
            }
        }

        private void OnResourceTypeChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (ResourceTypeComboBox.SelectedItem is System.Windows.Controls.ComboBoxItem item)
            {
                string type = item.Content.ToString() ?? "";
                if (string.IsNullOrEmpty(VirtualPathTextBox.Text) || VirtualPathTextBox.Text.EndsWith("/"))
                {
                    string basePath = type.ToLower() + "s/";
                    VirtualPathTextBox.Text = basePath;
                }
            }
        }

        private async void OnImport(object sender, RoutedEventArgs e)
        {
            if (_importService == null)
            {
                System.Windows.MessageBox.Show("Import service not available", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            string sourceFile = SourceFileTextBox.Text;
            if (string.IsNullOrEmpty(sourceFile) || !File.Exists(sourceFile))
            {
                System.Windows.MessageBox.Show("Please select a valid source file", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            string virtualPath = VirtualPathTextBox.Text;
            if (string.IsNullOrEmpty(virtualPath))
            {
                System.Windows.MessageBox.Show("Please enter a virtual path", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            ResourceType resourceType = GetSelectedResourceType();
            if (!_importService.ValidateImportFile(sourceFile, resourceType))
            {
                System.Windows.MessageBox.Show($"Selected file is not a valid {resourceType} file", "Error", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                return;
            }

            StatusTextBlock.Text = "Importing resource...";
            StatusTextBlock.Foreground = System.Windows.Media.Brushes.Yellow;

            bool success = await _importService.ImportResourceAsync(sourceFile, virtualPath, resourceType);
            
            if (success)
            {
                StatusTextBlock.Text = "Resource imported successfully!";
                StatusTextBlock.Foreground = System.Windows.Media.Brushes.Green;
                DialogResult = true;
                Close();
            }
            else
            {
                StatusTextBlock.Text = "Failed to import resource. Check console for details.";
                StatusTextBlock.Foreground = System.Windows.Media.Brushes.Red;
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
                string type = item.Content.ToString() ?? "Texture";
                return Enum.Parse<ResourceType>(type);
            }
            return ResourceType.Texture;
        }

        private string FormatFileSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB" };
            double len = bytes;
            int order = 0;
            while (len >= 1024 && order < sizes.Length - 1)
            {
                order++;
                len = len / 1024;
            }
            return $"{len:0.##} {sizes[order]}";
        }
    }
}
