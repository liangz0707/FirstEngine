using System.Windows;
using System.Windows.Controls;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.Views.Panels
{
    public partial class RenderView : UserControl
    {
        public RenderView()
        {
            InitializeComponent();
        }

        private void OnStartRender(object sender, RoutedEventArgs e)
        {
            if (DataContext is RenderViewModel viewModel)
            {
                viewModel.StartRendering();
            }
        }

        private void OnPauseRender(object sender, RoutedEventArgs e)
        {
            // TODO: Implement pause
        }

        private void OnStopRender(object sender, RoutedEventArgs e)
        {
            if (DataContext is RenderViewModel viewModel)
            {
                viewModel.StopRendering();
            }
        }
    }
}
