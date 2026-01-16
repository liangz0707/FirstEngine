using FirstEngineEditor.Services;
using FirstEngineEditor.ViewModels.Panels;

namespace FirstEngineEditor.ViewModels
{
    public class MainViewModel : ViewModelBase
    {
        private readonly IProjectManager _projectManager;
        private readonly IFileManager _fileManager;

        public MainViewModel()
        {
            _projectManager = ServiceLocator.Get<IProjectManager>();
            _fileManager = ServiceLocator.Get<IFileManager>();

            SceneListViewModel = new SceneListViewModel();
            ResourceListViewModel = new ResourceListViewModel();
            PropertyPanelViewModel = new PropertyPanelViewModel();
            RenderViewModel = new RenderViewModel();
        }

        public SceneListViewModel SceneListViewModel { get; }
        public ResourceListViewModel ResourceListViewModel { get; }
        public PropertyPanelViewModel PropertyPanelViewModel { get; }
        public RenderViewModel RenderViewModel { get; }

        public string WindowTitle
        {
            get
            {
                var projectName = _projectManager.CurrentProject?.Name ?? "Untitled";
                return $"FirstEngine Editor - {projectName}";
            }
        }
    }
}
