#pragma once
#include "IMovieSceneCapture.h"
#include "MovieSceneCaptureModule.h"
#include "Interfaces/IMineCustomToolModuleInterface.h"
#include "MineMouduleDefine.h"
#include "Slate/SceneViewport.h"

#define LOCTEXT_NAMESPACE "MineCustomToolCommands"

namespace MineCustomToolCommands
{
    class UserCommandsRegister : public IMineCustomToolModuleListenerInterface
    {
    private:
        IConsoleCommand *UserStartMovieCapture = nullptr;

    public:
        virtual  ~UserCommandsRegister () override {};

		virtual void OnStartupModule () override
		{
            UserStartMovieCapture =
                IConsoleManager::Get ().RegisterConsoleCommand (
                    TEXT ("MineStartMovieCapture"), TEXT ("MineStartMovieCapture"),
                    FConsoleCommandDelegate::CreateStatic (&FUserStartMovieCapture::ExecCommand
                    ), ECVF_Default
                );
		};

		virtual void OnShutdownModule () override
		{
            if (UserStartMovieCapture!= nullptr) {
                IConsoleManager::Get ().UnregisterConsoleObject (UserStartMovieCapture);
                UserStartMovieCapture = nullptr;
            }
		};

        class FUserStartMovieCapture
        {
        public:
            static void ExecCommand ()
            {
                auto RetVal = Exec();
                if (!RetVal){
                    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Can not start capture movie ?"))
                }
            }
            static bool Exec ()
            {

#if WITH_EDITOR
                
				IMovieSceneCaptureInterface *CaptureInterface = IMovieSceneCaptureModule::Get ().GetFirstActiveMovieSceneCapture ();
				if (CaptureInterface) {
					CaptureInterface->StartCapturing ();
					return true;
				}


				for (const FWorldContext &Context : GEditor->GetWorldContexts ()) {
					if (Context.WorldType == EWorldType::PIE) {
						FSlatePlayInEditorInfo *SlatePlayInEditorSession = GEditor->SlatePlayInEditorMap.Find (Context.ContextHandle);
						if (SlatePlayInEditorSession && SlatePlayInEditorSession->SlatePlayInEditorWindowViewport.IsValid ()) {
                            auto const SceneViewport = SlatePlayInEditorSession->SlatePlayInEditorWindowViewport;
                            // Focus on current viewport
                            SceneViewport->CaptureMouse(true);
                            auto const CurViewport = SceneViewport->GetViewport ();
                            CurViewport->SetUserFocus (true);

							IMovieSceneCaptureModule::Get ().CreateMovieSceneCapture (SlatePlayInEditorSession->SlatePlayInEditorWindowViewport.ToSharedRef ());
							return true;
						}
					}
				}

#endif
				return false;
            }
        };


    };
}


#undef LOCTEXT_NAMESPACE