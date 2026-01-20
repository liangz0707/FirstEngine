using System.Windows;

namespace FirstEngineEditor
{
    public partial class App : System.Windows.Application
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
