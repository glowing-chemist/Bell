**Bell:**

Bell is a modern rendergraph driven vulkan renderer. It supports both a UE4 style metalness/Roughess as well as a Specular/Gloss PBR pipeline.

![Material comparison](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/materialBottles.png)

It currently has forward and deferred image based lighting paths, af well as dynamic lighting provided by a clustered lighting path.
![Clustered lighting in Sponza](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/ClusteredSponza.png)

The renderer is trivially extendable by implementing a subclass of RenderEngine/Technique, which takes care of requesting 
per frame resources and implicitly specifying dependancies between tasks.

This repo also contains a standalone, node based editor which is rendered in engine.
pictured below is a forward IBL render graph in this editor.

![Node editor showing forward IBLcrendering](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/NodeDeferred.png)

And the frame it produces in the scene preview window.

![Scene preview showing forward IBLcrendering](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/helmet.png)

![Animation](https://github.com/glowing-chemist/Bell/blob/master/Assets/Screenshots/skinning.gif)
