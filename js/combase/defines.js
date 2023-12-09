
module.exports = class SERI_HEADER_BASE
{
    constructor()
    {
        this.dwClsid = 0;
        this.dwSize = 0;
        this.bVersion = 1;
        this.bReserved = 0;
    }

    static GetSeriSize()
    { return 12 }
}