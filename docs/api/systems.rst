Systems
=======

.. doxygenclass:: Eden::ISystem
   :members:

.. doxygenclass:: Eden::Systems::ScriptSystem
   :members:

.. doxygenclass:: Eden::Systems::InputSystem
   :members:

.. doxygenenum:: Eden::Input::Key

.. doxygenclass:: Eden::Systems::TransformSystem
   :members:

.. doxygenclass:: Eden::Systems::RenderSystem
   :members:

.. doxygenclass:: Eden::Rendering::Renderer
   :members:

Renderer types
--------------

.. doxygenfile:: RendererTypes.hpp

Culling
-------

.. doxygenstruct:: Eden::Rendering::AABB
   :members:

.. doxygenfunction:: Eden::Rendering::computeBounds

.. doxygenclass:: Eden::Rendering::Frustum
   :members:

Rendering components
---------------------

.. doxygenstruct:: Eden::Rendering::Components::Renderable
   :members:

.. doxygenstruct:: Eden::Rendering::Components::TintOverride
   :members:

.. doxygenstruct:: Eden::Rendering::Components::Camera
   :members:

Material
--------

.. doxygenstruct:: Eden::Rendering::Material
   :members:

Model
-----

.. doxygenstruct:: Eden::Rendering::Model
   :members:

.. doxygenstruct:: Eden::Rendering::ModelPart
   :members:

Script components
------------------

.. doxygenclass:: Eden::Scripting::ScriptBehaviour
   :members:

.. doxygenstruct:: Eden::Scripting::Components::ScriptComponent
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

.. doxygenclass:: Eden::Rendering::NullRenderer
   :members:
