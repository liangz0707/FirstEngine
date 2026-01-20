using System.Windows;
using System.Windows.Controls;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Views.Panels
{
    public partial class ResourceListView : System.Windows.Controls.UserControl
    {
        public ResourceListView()
        {
            InitializeComponent();
        }

        private void OnRefresh(object sender, RoutedEventArgs e)
        {
            if (DataContext is ResourceListViewModel viewModel)
            {
                viewModel.RefreshResources();
            }
        }
    }
}
