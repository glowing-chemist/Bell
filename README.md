**Bell:**

Bell is a modern rendergraph driven vulkan renderer. It implements a UE4 style metalness/Roughess PBR pipeline.
It currently has forward and deferred image based lighting paths (with a clustered lighting path being worked on).

The renderer is trivially extendable by implementing a subclass of Engine/Technique, which takes care of requesting 
per frame resources and implicitly specifying dependancies between tasks.

This repo also contains a standalone, node based editor which is rendered in engine.
pictured below is a forward IBL render graph in this editor.

![Node editor showing forward IBLcrendering](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/BellEditorGunNodesForward.png)

And the frame it produces in the scene preview window.

![Scene preview showing forward IBLcrendering](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/BellEditorGun.png)
