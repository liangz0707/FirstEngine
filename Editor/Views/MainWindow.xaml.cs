using System.Windows;
using System.IO;
using FirstEngineEditor.Services;

namespace FirstEngineEditor.Views
{
    public partial class MainWindow : Window
    {
        private readonly IProjectManager _projectManager;

        public MainWindow()
        {
            InitializeComponent();
            _projectManager = ServiceLocator.Get<IProjectManager>();
        }

        private void OnNewProject(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select project folder"
            };

            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                // Simple input dialog using MessageBox
                var inputDialog = new InputDialog("Enter project name:", "New Project", "NewProject");
                if (inputDialog.ShowDialog() == true)
                {
                    var projectName = inputDialog.Answer;
                    if (!string.IsNullOrEmpty(projectName))
                    {
                        var projectPath = Path.Combine(dialog.SelectedPath, projectName);
                        _projectManager.CreateProject(projectPath, projectName);
                    }
                }
            }
        }

        private void OnOpenProject(object sender, RoutedEventArgs e)
        {
            var dialog = new System.Windows.Forms.FolderBrowserDialog
            {
                Description = "Select project folder"
            };

            if (dialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                bool opened = _projectManager.OpenProject(dialog.SelectedPath);
                if (!opened)
                {
                    System.Windows.MessageBox.Show($"Failed to open project at: {dialog.SelectedPath}", "Error", 
                        System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
                }
                else
                {
                    var project = _projectManager.CurrentProject;
                    System.Diagnostics.Debug.WriteLine($"Project opened: {project?.ProjectPath}");
                    Console.WriteLine($"Project opened: {project?.ProjectPath}");
                    
                    // Verify Package directory
                    if (project != null)
                    {
                        string packagePath = System.IO.Path.Combine(project.ProjectPath, "Package");
                        if (System.IO.Directory.Exists(packagePath))
                        {
                            string manifestPath = System.IO.Path.Combine(packagePath, "resource_manifest.json");
                            if (System.IO.File.Exists(manifestPath))
                            {
                                System.Windows.MessageBox.Show($"Project opened successfully!\nPackage found at: {packagePath}\nManifest: {manifestPath}", 
                                    "Project Opened", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
                            }
                            else
                            {
                                System.Windows.MessageBox.Show($"Project opened, but resource_manifest.json not found at: {manifestPath}", 
                                    "Warning", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                            }
                        }
                        else
                        {
                            System.Windows.MessageBox.Show($"Project opened, but Package directory not found at: {packagePath}", 
                                "Warning", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Warning);
                        }
                    }
                }
            }
        }

        private void OnSave(object sender, RoutedEventArgs e)
        {
            _projectManager.SaveProject();
        }

        private void OnSaveAs(object sender, RoutedEventArgs e)
        {
            // TODO: Implement save as
        }

        private void OnExit(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void OnImportResource(object sender, RoutedEventArgs e)
        {
            var dialog = new Views.ResourceImportDialog();
            dialog.ShowDialog();
        }

        private void OnCreateResource(object sender, RoutedEventArgs e)
        {
            var dialog = new Views.CreateResourceDialog();
            dialog.ShowDialog();
        }
    }
}
