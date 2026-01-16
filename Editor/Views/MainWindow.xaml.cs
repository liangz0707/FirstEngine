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
                _projectManager.OpenProject(dialog.SelectedPath);
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
    }
}
