#include <limits>
#include <boost/asio.hpp>

#include "../../../../thirdparty/boost/1.55.0/dist/boost/lexical_cast.hpp"
#include "../../../../thirdparty/boost/1.55.0/dist/boost/mpl/remove_if.hpp"
#include "../../../../thirdparty/boost/1.55.0/dist/boost/mpl/for_each.hpp"
#include "../../../../thirdparty/boost/1.55.0/dist/boost/mpl/range_c.hpp"
#include "../../../../thirdparty/boost/1.55.0/dist/boost/fusion/container/vector.hpp"
#include <iostream>

// Abstract checker interface
class IChecker
{
public:
    // Also all subclasses must implement the following static function
    //          static std::string getTypeName();
    virtual void setAttributeName(const std::string &newName) = 0;
    virtual void setMinValue(const std::string &newMinValue) = 0;
    virtual void setMaxValue(const std::string &newMaxValue) = 0;
    virtual bool inLimits(const Attributes &attrs) const = 0;
};

class VPChecker: public IChecker
{
public:
    explicit VPChecker(std::string attrName)
        : m_attrIndex{std::move(attrName)}
    {
    };

    static std::string getTypeName() 
    {
        return "VanishingPoint";
    }

    void setAttributeName(const std::string &newName) override
    {
        m_attrIndex = AttributeIndex<CPluginVanishingPoint, Read, DirectAccess>{newName};
    }

    void setMinValue(const std::string &newMinValue) override
    {
        std::vector<float> values;
        tools::VAL(newMinValue, values, ",");
        m_minValue.x = values[0];
        m_minValue.y = values[1];
    }

    void setMaxValue(const std::string &newMaxValue) override
    {
        std::vector<float> values;
        tools::VAL(newMaxValue, values, ",");
        m_maxValue.x = values[0];
        m_maxValue.y = values[1];
    }

    bool inLimits(const Attributes &attrs) const override
    {
        const auto &attr = attrs.access(m_attrIndex);

        return attr.x >= m_minValue.x && attr.y >= m_minValue.x && 
               attr.x <= m_maxValue.x && attr.y <= m_minValue.y;
    }

private:
    AttributeIndex<CPluginVanishingPoint, Read, DirectAccess> m_attrIndex;
    CPluginVanishingPoint m_minValue;
    CPluginVanishingPoint m_maxValue;
};


template<class T>
class SimpleDataChecker: public IChecker
{
public:
    explicit SimpleDataChecker(std::string attrName)
        : m_attrIndex{std::move(attrName)}
    {
    }

    static std::string getTypeName()
    {
        return typeid(T).name();
    }

    void setAttributeName(const std::string &newName) override
    {
        m_attrIndex = AttributeIndex<CDataSimple<T>, Read, StrippedAccess>{newName};
    }

    void setMinValue(const std::string &newMinValue) override
    {
        m_minValue = boost::lexical_cast<T>(newMinValue);
    }

    virtual void setMaxValue(const std::string &newMaxValue) override
    {
        m_maxValue = boost::lexical_cast<T>(newMaxValue);
    }

    bool inLimits(const Attributes &attrs) const override
    {
        const auto &attr = attrs.access(m_attrIndex);

        return attr >= m_minValue && attr <= m_maxValue;
    }

private:
    AttributeIndex<CDataSimple<T>, Read, StrippedAccess> m_attrIndex;
    T m_minValue;
    T m_maxValue;
};

#include "registration.h"

#define CHECKER_TO_REGISTER SimpleDataChecker<double>
#include REGISTER_CHECKER()

#define CHECKER_TO_REGISTER VPChecker
#include REGISTER_CHECKER()

// Always need to iterate over indexes because otherwise default and copy constructors must be 
// implemented for all the checker classes.
typedef boost::mpl::range_c<unsigned int, 0, boost::mpl::size<CHECKERS_LIST()>::value> checkersIndexes;

struct createChecker
{
    typedef void result_type;
    static std::unique_ptr<IChecker> checker;
    template<typename U> void operator()(int i, std::string attrName, U)
    {
        if (i == U::value)
        {
            checker = std::make_unique<boost::mpl::at_c<CHECKERS_LIST(), U::value>::type>(std::move(attrName));
        }
    }
};
std::unique_ptr<IChecker> createChecker::checker;

struct createListOfTypes
{
    typedef void result_type;
    static std::ostringstream listOfTypes;
    template<typename U> void operator()(U)
    {
        typedef boost::mpl::at_c<CHECKERS_LIST(), U::value>::type checkerType;
        listOfTypes << "[" << checkerType::getTypeName() << " " << U::value << "]";
    }
};
std::ostringstream createListOfTypes::listOfTypes;

//////////////////////////////////////////////////////////////////////////
// Plugin Value Checker

class CPluginValueChecker
{
public:
    explicit CPluginValueChecker(bool isFactoryInstance = false);

private:
    void addSlots();
    void initPlugin();
    void InterfaceDefinition(CInterface &iface) override;

    void onData(Attributes * attrs);

    void checkedAttrNameChanged(std::string newRegionAttrName);
    void resultAttrNameChanged(std::string newDefectAttrName);

    void minValueChanged(std::string newMinValue);
    void maxValueChanged(std::string newMaxValue);

    void dataTypeChanged(int newDataType);

    std::string m_checkedAttrName;

    std::string m_resultAttrName;
    AttributeIndex<CPluginBool, Create, StrippedAccess> m_resultAttrIndex;

    std::string m_minValue;
    std::string m_maxValue;

    int m_dataType;

} REGISTER_PLUGIN(CPluginValueChecker);

CPluginValueChecker::CPluginValueChecker(bool isFactoryInstance)
, m_checkedAttrName("region")
, m_resultAttrName("defect")
, m_resultAttrIndex{m_resultAttrName}
, m_minValue("0.0")
, m_maxValue("0.0")
{
    addSlots();
    initPlugin();

    if (!isFactoryInstance)
    {
    }
}

void CPluginValueChecker::initPlugin()
{
    InitPlugin(
        "Value Checker",
        "Checks that specified attribute value and sets the attribute to TRUE if the value matches criteria.",
        __DATE__,);
}

void CPluginValueChecker::addSlots()
{
    const auto inputSlotId = AddInputSlot(
        "ATTRIBUTES",
        "Attributes.",
        "Attrs");

    const auto outputSlotId = AddOutputSlot(
        "ATTRIBUTES",
        "Same as input attributes + result set to True/False.",
        "Attrs");

    AddIO1(CPluginValueChecker::onData, inputSlotId, outputSlotId, Attributes, "");
}

void CPluginValueChecker::InterfaceDefinition(CInterface &iface)
{
    // Create a list of type strings
    createListOfTypes::listOfTypes.str(std::string());
    createListOfTypes::listOfTypes << "[";
    //boost::mpl::for_each<CHECKERS_LIST()>(createListOfTypes());
    boost::mpl::for_each<checkersIndexes>(createListOfTypes());
    createListOfTypes::listOfTypes << "]";

    publish_cb1_p(
        iface,
        this,
        &CPluginValueChecker::dataTypeChanged,
        m_dataType,
        "Data Type",
        createListOfTypes::listOfTypes.str(),
        "Supported data types.",
        "0");

    publish_cb1_p(
        iface,
        this,
        &CPluginValueChecker::checkedAttrNameChanged,
        m_checkedAttrName,
        "Checked Attribute",
        "",
        "Name of attribute which value should be checked.",
        "");

    // \todo What do we do if such an attribute is already present?
    // Silently accept that and overwrite it? Warning? Error?
    publish_cb1_p(
        iface,
        this,
        &CPluginValueChecker::resultAttrNameChanged,
        m_resultAttrName,
        "Result Attribute",
        "",
        "Name of attribute which contains the result of the check.",
        "");

    publish_cb1_p(
        iface,
        this,
        &CPluginValueChecker::maxValueChanged,
        m_maxValue,
        "Max Value",
        "",
        "Maximum allowed attribute value.",
        "");

    publish_cb1_p(
        iface,
        this,
        &CPluginValueChecker::minValueChanged,
        m_minValue,
        "Min Value",
        "",
        "Minimum allowed attribute value.",
        "");
}

void CPluginValueChecker::onData(Attributes * attrs)
{
    const bool compRes = createChecker::checker->inLimits(*attrs);
    attrs->store(m_resultAttrIndex, compRes);
}

void CPluginValueChecker::checkedAttrNameChanged(std::string newCheckedAttrName)
{
    if (!newCheckedAttrName.empty())
    {
        createChecker::checker->setAttributeName(newCheckedAttrName);
        m_checkedAttrName = std::move(newCheckedAttrName);
    }
}

void CPluginValueChecker::resultAttrNameChanged(std::string newDefectAttrName)
{
    if (!newDefectAttrName.empty())
    {
        m_resultAttrName = std::move(newDefectAttrName);
        m_resultAttrIndex = AttributeIndex<CPluginBool, Create, StrippedAccess>{m_resultAttrName};
    }
}

void CPluginValueChecker::maxValueChanged(std::string newMaxValue)
{
    if (!newMaxValue.empty())
    {
        createChecker::checker->setMaxValue(newMaxValue);
        m_maxValue = newMaxValue;
    }
}

void CPluginValueChecker::minValueChanged(std::string newMinValue)
{
    if (!newMinValue.empty())
    {
        createChecker::checker->setMinValue(newMinValue);
        m_minValue = newMinValue;
    }
}

void CPluginValueChecker::dataTypeChanged(int newDataType)
{
    boost::mpl::for_each<checkersIndexes>(boost::bind(createChecker(), newDataType, m_checkedAttrName, _1));
    m_dataType = newDataType;
}