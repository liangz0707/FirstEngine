using FirstEngineEditor.ViewModels;

namespace FirstEngineEditor.ViewModels.Panels
{
    public class RenderViewModel : ViewModelBase
    {
        private bool _isRendering;
        private string _renderStatus = "Ready";

        public RenderViewModel()
        {
            IsRendering = false;
        }

        public bool IsRendering
        {
            get => _isRendering;
            set => SetProperty(ref _isRendering, value);
        }

        public string RenderStatus
        {
            get => _renderStatus;
            set => SetProperty(ref _renderStatus, value);
        }

        public void StartRendering()
        {
            IsRendering = true;
            RenderStatus = "Rendering...";
        }

        public void StopRendering()
        {
            IsRendering = false;
            RenderStatus = "Stopped";
        }
    }
}
