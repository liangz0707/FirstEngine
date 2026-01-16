using System.Windows;

namespace FirstEngineEditor
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            
            // Initialize services
            Services.ServiceLocator.Initialize();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // Cleanup services
            Services.ServiceLocator.Shutdown();
            
            base.OnExit(e);
        }
    }
}
