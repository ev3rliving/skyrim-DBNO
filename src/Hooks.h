#pragma once

namespace Hooks
{
    void Install();

    namespace
    {
        RE::NiAVObject* Load3D(RE::Character& a_this, bool a_arg1);
        inline REL::Relocation<decltype(Load3D)>              _Load3D;
    }
}
