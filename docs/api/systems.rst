Systems
=======

.. doxygenclass:: Eden::ISystem
   :members:

.. doxygenclass:: Eden::RenderSystem
   :members:

.. doxygenclass:: Eden::Renderer
   :members:

Renderer types
--------------

.. doxygenfile:: RendererTypes.hpp

Vulkan backend
--------------

The only Renderer implementation today. Documented here for completeness;
treat it as an implementation detail behind the Renderer contract above,
not something calling code should depend on directly.

.. doxygenclass:: Eden::VulkanRenderer
   :members:
   :private-members:
