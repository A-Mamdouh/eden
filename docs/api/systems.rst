Systems
=======

.. doxygenclass:: Eden::ISystem
   :members:

.. doxygenclass:: Eden::ScriptSystem
   :members:

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

.. doxygenenum:: Eden::PrimitiveShape

.. doxygenstruct:: Eden::Renderable
   :members:

Script components
------------------

.. doxygenclass:: Eden::ScriptBehaviour
   :members:

.. doxygenstruct:: Eden::ScriptComponent
   :members:

Vulkan backend
--------------

The only Renderer implementation today. Documented here for completeness;
treat it as an implementation detail behind the Renderer contract above,
not something calling code should depend on directly.

.. doxygenclass:: Eden::VulkanRenderer
   :members:
   :private-members:

NullRenderer
------------

A second, headless ``Renderer`` implementation -- no window, no GPU, no
graphics API calls. It exists to prove the contract above is genuinely
backend-agnostic rather than a one-implementation abstraction, and to
let engine logic be unit-tested without a GPU (see ``tests/``). Not
wired into ``RenderSystem``; tests construct one directly.

.. doxygenclass:: Eden::NullRenderer
   :members:
