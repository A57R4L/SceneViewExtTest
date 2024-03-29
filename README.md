### Update for Unreal Engine 5

Please find an up-to-date plugin repository here: [https://github.com/A57R4L/SceneViewExtensionTemplate](https://github.com/A57R4L/SceneViewExtensionTemplate)

# SceneViewExtTest
 UE 4.26 (works now on 4.27 and 5.0.1) test project and plugin for SceneViewExtension and how to hook custom shaders (Vertex Shader, Pixel Shader and Compute Shader) to RDG/GraphBuilder

I had really hard time finding up to date documentation on how to use custom shaders and access the post process chain in Unreal Engine without modifying the engine code. This is an attempt to document the very basic process to setup a custom hook to the SceneView and some basic shaders.

This repository holds a template UE4 project with a single empty actor that has a SceneViewComponent that hooks a custom Vertex&Pixel shader to the Post Process chain. All the code lies in the plugin folder. It takes care of the Pixel and Vertex shader loading (a compute or geometry shader with this technique shouldn't be that difficult either). Then injects those to the post process chain via SceneViewExtensionBase class that gives the access to the RDG/GraphBuilder. As the Engine code is constantly evolving, things might get broken for the next version. However this is already a higher level implementation than direct RHI/DirectX access, so this should be easier to update if/when the engine code changes again. 

This is for the Win64 platform and can't verify how easily transfers to other platform.

Most of the insights came from the Engine code - especially Pixel Inspector and IO Color Profile Extension. A hugh shout out to so many people asking and contributing with various small insights on the UE forum and Unreal Slackers Discord especially. For me just to get this actually working was a quest for several months, so this repository was made to hopefully easen the path for someone else. And also to gather feedback to further improve the implementation and best practices.

NOTE ON THE ACTUAL UE4 PROJECT/FULL REPOSITORY: When hitting play the viewport blue and green components of the scenecolor are reversed as a proof of concept, otherwise it is just a vanilla empty c++ project template from the engine. Plugin folder should work as a copy/paste to any UE4.26> project.

UPDATE May 2022: 
* Some refractoring and #if MAJOR_ENGINE_VERSION == 5 modifications to highlight changes from UE4.26->UE4.27->UE5.x
* Alternative example to hook a compute shader instead of Vertex and Pixel Shader. There are some additional features in UE5 to affect the screen percentage at various points of rendering, and I'm not quite sure everything is properly in place yet at the engine side - see the comments on different ways to get the right viewport size depending on at what point of the post processing chain you plan to implement your actual effect
* You can change the used shader method with console command: r.SceneVETest.Shader
