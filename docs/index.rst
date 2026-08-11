Eden
====

Eden is a C++20 graphics engine. It uses Vulkan as its current rendering
backend, but the architecture is meant to support additional backends
(e.g. a native Metal backend for Apple Silicon) behind a shared
:cpp:class:`Eden::Renderer` contract. It is not a game engine: there is
no physics, audio, or gameplay-scripting layer, and none is planned as
core scope.

Start with :doc:`architecture` if you're getting back into this codebase
after time away -- it covers what's actually wired up versus what's
aspirational, before you go read source.

.. toctree::
   :maxdepth: 2
   :caption: Contents

   architecture
   api/core
   api/services
   api/systems
