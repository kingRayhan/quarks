#if !defined(SORTER__INCLUDED_)
#define SORTER__INCLUDED_

#include <set>
#include <vector>

namespace Sorter {
    
     struct StringValuesWithJson{
        crow::json::wvalue& _value;
        //std::string _value;
        std::string _sortby;
        
        StringValuesWithJson(crow::json::wvalue& value, std::string sortby)
            : _value(value), _sortby(sortby){
            
        }
    };
    
    struct NumberValuesWithJson{
        crow::json::wvalue& _value;
        double _sortby;
        
        NumberValuesWithJson(crow::json::wvalue& value, double sortby)
        :  _value(value), _sortby(sortby){
        }
        
    };
    
    struct StringComparer
    {
        bool operator () (const StringValuesWithJson& lhs, const StringValuesWithJson& rhs)
        {
            return (lhs._sortby.compare(rhs._sortby)) < 0 ? true : false;
        }
    };
    
    struct NumberComparer
    {
        bool operator () (const NumberValuesWithJson& lhs, const NumberValuesWithJson& rhs)
        {
            return lhs._sortby < rhs._sortby;
        }
    };
    
    struct JsonComparer
    {
        JsonComparer(std::string sorter, bool ascending)
            :_sorter(sorter), _ascending(ascending){}
        
        
        bool operator () (const crow::json::rvalue& lhs, const crow::json::rvalue& rhs)
        {
            bool ret = false;
         
            try{
                // 0 for string, 1 for numeric
                int typeLhs = -1;
                int typeRhs = -2;
                
                if(lhs.has(_sorter) && rhs.has(_sorter)){
                    auto lSorter = lhs[_sorter];
                    auto rSorter = rhs[_sorter];
                    
                    switch (lSorter.t()) {
                        case crow::json::type::String:{
                            typeLhs = 0;
                            break;
                        }
                            
                        case crow::json::type::Number:{
                            typeLhs = 1;
                            break;
                        }
                        
                        default:;
                    }
                    
                    switch (rSorter.t()) {
                        case crow::json::type::String:{
                            typeRhs = 0;
                            break;
                        }
                            
                        case crow::json::type::Number:{
                            typeRhs = 1;
                            break;
                        }
                            
                        default:;
                    }
                    
                    if(typeLhs == typeRhs){
                        if(typeLhs == 1){
                            ret = compareNumeric(lSorter, rSorter);
                        }else if(typeLhs == 0){
                            ret = compareAlpha(lSorter, rSorter);
                        }
                    }
                }
                
            }catch (const std::runtime_error& error){
                CROW_LOG_INFO << "Runtime Error while sorting: " << error.what();
                
                ret = false;
                
            };
            
            
            if(!_ascending){
                ret = !ret;
            }
            
            return ret;
        }
        
        bool compareAlpha (const crow::json::rvalue& lhs, const crow::json::rvalue& rhs)
        {
            std::string ls = lhs.s();
            std::string rs = rhs.s();
            
            return (ls.compare(rs) < 0) ? true : false;
            
        }
        
        bool compareNumeric(const crow::json::rvalue& lhs, const crow::json::rvalue& rhs)
        {
            double ld = lhs.d();
            double rd = rhs.d();
            
            return ld < rd;
        }
        
        std::string _sorter;
        bool _ascending;
    };
    
    typedef std::set<StringValuesWithJson, StringComparer> SetAlpha;
    typedef std::set<NumberValuesWithJson, NumberComparer> SetNumeric;
    
    //typedef SetAlpha::const_iterator itAlpha;
    //typedef SetNumeric::const_iterator itNumeric;
    
}


#endif // !defined(SORTER__INCLUDED_)
