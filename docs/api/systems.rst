Systems
=======

.. doxygenclass:: Eden::ISystem
   :members:

.. doxygenclass:: Eden::ScriptSystem
   :members:

.. doxygenclass:: Eden::InputSystem
   :members:

.. doxygenenum:: Eden::Key

.. doxygenclass:: Eden::TransformSystem
   :members:

.. doxygenclass:: Eden::RenderSystem
   :members:

.. doxygenclass:: Eden::Renderer
   :members:

Renderer types
--------------

.. doxygenfile:: RendererTypes.hpp

Renderable component
---------------------

.. doxygenstruct:: Eden::Renderable
   :members:

.. doxygenstruct:: Eden::TintOverride
   :members:

Material
--------

.. doxygenstruct:: Eden::Material
   :members:

Model
-----

.. doxygenstruct:: Eden::Model
   :members:

.. doxygenstruct:: Eden::ModelPart
   :members:

Script components
------------------

.. doxygenclass:: Eden::ScriptBehaviour
   :members:

.. doxygenstruct:: Eden::ScriptComponent
   :members:

Vulkan backend
--------------

The renderer implementation actually driving pixels: ``VulkanRenderer``
under ``src/Systems/RenderSystem/Vulkan/``. It's intentionally outside
this generated reference -- its header lives in ``src/``, not
``include/``, specifically so no Vulkan type is reachable from the
public API surface at all, and Doxygen only scans ``include/Eden``. See
the source directly, or the "Backend header firewall" section of
:doc:`../architecture` for why it's organized this way.

glTF loading
------------

``Engine::loadModel()`` (documented above under Core) is the public
entry point -- it returns a ``Model`` (above) for the caller to attach to
an entity. The cgltf/stb_image-based implementation
(``src/Systems/RenderSystem/GltfLoader.cpp``) is intentionally outside
this reference for the same reason as the Vulkan backend -- it's a
private implementation detail (``RenderSystem::loadModel()``, not part
of ``include/Eden``), not something a consumer is meant to reach into.

NullRenderer
------------

A second, headless ``Renderer`` implementation -- no window, no GPU, no
graphics API calls. It exists to prove the contract above is genuinely
backend-agnostic rather than a one-implementation abstraction, and to
let engine logic be unit-tested without a GPU (see ``tests/``). Not
wired into ``RenderSystem``; tests construct one directly.

.. doxygenclass:: Eden::NullRenderer
   :members:
