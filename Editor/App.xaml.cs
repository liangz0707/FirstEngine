using System.Windows;

namespace FirstEngineEditor
{
    public partial class App : System.Windows.Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            // Debug test - 在这里设置断点
            var debugTest = 123;
            System.Diagnostics.Debug.WriteLine("OnStartup called!");
            
            base.OnStartup(e);
            
            // Initialize services
            Services.ServiceLocator.Initialize();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // Cleanup services
            Services.ServiceLocator.Shutdown();
            
            // Shutdown global render engine
            Services.RenderEngineService.ShutdownGlobalEngine();
            
            base.OnExit(e);
        }
    }
}
