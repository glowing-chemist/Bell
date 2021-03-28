#include "DX_12BufferView.hpp"
#include "DX_12Buffer.hpp"


DX_12BufferView::DX_12BufferView(Buffer& buf, const uint64_t offset, const uint64_t size) :
    BufferViewBase(buf, offset, size)
{
    DX_12Buffer* DXBuffer = static_cast<DX_12Buffer*>(buf.getBase());
    mResource = DXBuffer->getResourceHandle();
    mResource->AddRef();
}

DX_12BufferView::~DX_12BufferView()
{
    mResource->Release();
}