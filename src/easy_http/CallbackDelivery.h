#pragma once

namespace ezhttp
{
    enum class CallbackDelivery { All, OptedIn, Suspended };
    using GetCallbackDelivery = CallbackDelivery (*)();
}
