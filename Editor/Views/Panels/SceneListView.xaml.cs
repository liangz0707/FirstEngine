using System.Windows;
using System.Windows.Controls;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Views.Panels
{
    public partial class SceneListView : System.Windows.Controls.UserControl
    {
        public SceneListView()
        {
            InitializeComponent();
        }

        private void OnAddScene(object sender, RoutedEventArgs e)
        {
            // TODO: Implement add scene
        }

        private void OnRemoveScene(object sender, RoutedEventArgs e)
        {
            if (DataContext is SceneListViewModel viewModel && viewModel.SelectedScene != null)
            {
                // TODO: Implement remove scene
            }
        }

        private void OnRefresh(object sender, RoutedEventArgs e)
        {
            if (DataContext is SceneListViewModel viewModel)
            {
                viewModel.RefreshScenes();
            }
        }
    }
}
