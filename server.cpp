
#include<iostream>



#include "collision_manager/collision.hpp"
#include "collision_manager/collision_manager.hpp"
#include "collision_manager/query.hpp"
#include <filesystem>


#include<collision.grpc.pb.h>
#include<grpcpp/grpcpp.h>

using Value = std::variant<float, size_t, std::string, std::chrono::year_month_day, std::chrono::hh_mm_ss<std::chrono::minutes>, std::uint8_t, std::uint32_t>;

static std::unique_ptr<CollisionManager> collision_manager = std::make_unique<CollisionManager>(std::string("../MotorVehicleCollisionData_subset.csv"));

struct InternalQuery {

    public:

        InternalQuery(CollisionField f, QueryType t, Value v): field(f),type(t),val(v){}
        CollisionField field;
        QueryType type;
        Value val;

};


class QueryWrapper{

    public :

        static Query get_construted_query (const collision_proto::QueryRequest* request){

        std::vector<InternalQuery> queries;
        std::cout << "Request query size - " << request->queries().size() << std::endl;

        Value value;
        CollisionField field;
        QueryType type;

        for (const auto& item : request->queries()){

                field = get_match_field(item.field());
                type = get_match_condition(item.condition());

                if (field == CollisionField::BOROUGH){
                    value = item.str_data();


                }else if (field == CollisionField::ZIP_CODE){

                   int32_t  zip = item.int_data();
                   value = static_cast<uint32_t> (zip);
                }
                std::visit ([](auto&& arg){ std::cout<< arg << std::endl;},value);
                InternalQuery gq(field,type,value);
                queries.push_back(gq);
        }

        std::cout << "vector query size - " << queries.size() << std::endl;

        Query query = Query::create(queries[0].field , queries[0].type, queries[0].val);

        std::vector<InternalQuery>::iterator it;

        for ( it = queries.begin()+1 ; it != queries.end(); it++ ){
             query.add( it->field, it->type, it->val);
        }
        return query;

        }

        static void ConvertToProto(const CollisionProxy* data_struct, collision_proto::Collision*  proto_data) {

            // Convert crash date (assuming it's in the form std::chrono::year_month_day)

            /*
            if (data_struct->crash_date != std::chrono::year_month_day{}) {  // Check if the date is set
                std::ostringstream date_stream;
                date_stream << std::chrono::year(data_struct->crash_date) << "-"
                            << std::chrono::month(data_struct->crash_date) << "-"
                            << std::chrono::day(data_struct->crash_date);
                proto_data->set_crash_date(date_stream.str());
            }

            // Convert crash time (assuming it's in std::chrono::hh_mm_ss)
            if (data_struct->crash_time != std::chrono::hh_mm_ss<std::chrono::minutes>{}) {  // Check if the time is set
                std::ostringstream time_stream;
                time_stream << std::chrono::hours(data_struct->crash_time) << ":"
                            << std::chrono::minutes(data_struct->crash_time) << ":"
                            << std::chrono::seconds(data_struct->crash_time);
                proto_data->set_crash_time(time_stream.str());
            }*/

            // Convert optional fields using has_value() for std::optional

            if (data_struct->borough && data_struct->borough->has_value()) {
                proto_data->set_borough(data_struct->borough->value());
            }

            if (data_struct->zip_code && data_struct->zip_code->has_value()) {
                proto_data->set_zip_code(data_struct->zip_code->value());
            }

            if (data_struct->latitude && data_struct->latitude->has_value()) {
                proto_data->set_latitude(data_struct->latitude->value());
            }

            if (data_struct->longitude && data_struct->longitude->has_value()) {
                proto_data->set_longitude(data_struct->longitude->value());
            }

            if (data_struct->location && data_struct->location->has_value()) {
                proto_data->set_location(data_struct->location->value());
            }

            if (data_struct->on_street_name && data_struct->on_street_name->has_value()) {
                proto_data->set_on_street_name(data_struct->on_street_name->value());
            }

            if (data_struct->cross_street_name && data_struct->cross_street_name->has_value()) {
                proto_data->set_cross_street_name(data_struct->cross_street_name->value());
            }

            if (data_struct->off_street_name && data_struct->off_street_name->has_value()) {
                proto_data->set_off_street_name(data_struct->off_street_name->value());
            }

            if (data_struct->number_of_persons_injured && data_struct->number_of_persons_injured->has_value()) {
                proto_data->set_number_of_persons_injured(data_struct->number_of_persons_injured->value());
            }

            if (data_struct->number_of_persons_killed && data_struct->number_of_persons_killed->has_value()) {
                proto_data->set_number_of_persons_killed(data_struct->number_of_persons_killed->value());
            }

            if (data_struct->number_of_pedestrians_injured && data_struct->number_of_pedestrians_injured->has_value()) {
                proto_data->set_number_of_pedestrians_injured(data_struct->number_of_pedestrians_injured->value());
            }

            if (data_struct->number_of_pedestrians_killed && data_struct->number_of_pedestrians_killed->has_value()) {
                proto_data->set_number_of_pedestrians_killed(data_struct->number_of_pedestrians_killed->value());
            }

            if (data_struct->number_of_cyclist_injured && data_struct->number_of_cyclist_injured->has_value()) {
                proto_data->set_number_of_cyclist_injured(data_struct->number_of_cyclist_injured->value());
            }

            if (data_struct->number_of_cyclist_killed && data_struct->number_of_cyclist_killed->has_value()) {
                proto_data->set_number_of_cyclist_killed(data_struct->number_of_cyclist_killed->value());
            }

            if (data_struct->number_of_motorist_injured && data_struct->number_of_motorist_injured->has_value()) {
                proto_data->set_number_of_motorist_injured(data_struct->number_of_motorist_injured->value());
            }

            if (data_struct->number_of_motorist_killed && data_struct->number_of_motorist_killed->has_value()) {
                proto_data->set_number_of_motorist_killed(data_struct->number_of_motorist_killed->value());
            }

            if (data_struct->contributing_factor_vehicle_1 && data_struct->contributing_factor_vehicle_1->has_value()) {
                proto_data->set_contributing_factor_vehicle_1(data_struct->contributing_factor_vehicle_1->value());
            }

            if (data_struct->contributing_factor_vehicle_2 && data_struct->contributing_factor_vehicle_2->has_value()) {
                proto_data->set_contributing_factor_vehicle_2(data_struct->contributing_factor_vehicle_2->value());
            }

            if (data_struct->contributing_factor_vehicle_3 && data_struct->contributing_factor_vehicle_3->has_value()) {
                proto_data->set_contributing_factor_vehicle_3(data_struct->contributing_factor_vehicle_3->value());
            }

            if (data_struct->contributing_factor_vehicle_4 && data_struct->contributing_factor_vehicle_4->has_value()) {
                proto_data->set_contributing_factor_vehicle_4(data_struct->contributing_factor_vehicle_4->value());
            }

            if (data_struct->contributing_factor_vehicle_5 && data_struct->contributing_factor_vehicle_5->has_value()) {
                proto_data->set_contributing_factor_vehicle_5(data_struct->contributing_factor_vehicle_5->value());
            }

            if (data_struct->collision_id && data_struct->collision_id->has_value()) {
                proto_data->set_collision_id(data_struct->collision_id->value());
            }

            if (data_struct->vehicle_type_code_1 && data_struct->vehicle_type_code_1->has_value()) {
                proto_data->set_vehicle_type_code_1(data_struct->vehicle_type_code_1->value());
            }

            if (data_struct->vehicle_type_code_2 && data_struct->vehicle_type_code_2->has_value()) {
                proto_data->set_vehicle_type_code_2(data_struct->vehicle_type_code_2->value());
            }

            if (data_struct->vehicle_type_code_3 && data_struct->vehicle_type_code_3->has_value()) {
                proto_data->set_vehicle_type_code_3(data_struct->vehicle_type_code_3->value());
            }

            if (data_struct->vehicle_type_code_4 && data_struct->vehicle_type_code_4->has_value()) {
                proto_data->set_vehicle_type_code_4(data_struct->vehicle_type_code_4->value());
            }

            if (data_struct->vehicle_type_code_5 && data_struct->vehicle_type_code_5->has_value()) {
                proto_data->set_vehicle_type_code_5(data_struct->vehicle_type_code_5->value());
            }
        };

    private:

        static QueryType get_match_condition(collision_proto::Condition condition){

            if (condition == collision_proto::Condition::EQUALS){
                return QueryType::EQUALS;
            }
            std::cerr<< "Error" << std::endl;
            return {};
        };


        static CollisionField get_match_field(collision_proto::QueryFields field){

            if (field == collision_proto::QueryFields::BOROUGH){
                return CollisionField::BOROUGH;
            }else if (field == collision_proto::QueryFields::ZIP_CODE){
                return CollisionField::ZIP_CODE;
            }
            std::cerr<< "Error" << std::endl;
            return {};

        }
};



class CollisionQueryServiceImpl final : public collision_proto::CollisionQueryService::Service{

public:

    grpc::Status GetCollisions(grpc::ServerContext* context, const collision_proto::QueryRequest* request, collision_proto::QueryResponse* response) override{


        Query query = QueryWrapper::get_construted_query(request);
        std::vector<CollisionProxy*> collisions = collision_manager->searchOpenMp(query);

        std::cout << "collison size" << collisions.size() << std::endl;

        int counter= 100;
        for (auto proxy : collisions ){
            if (counter == 0){
                break;
            }
            collision_proto::Collision* collision = response->add_collision();
            QueryWrapper::ConvertToProto(proxy, collision);
            counter--;
        }

        return grpc::Status::OK;
    }

};

void RunServer(){

    std::string server_address("0.0.0.0:50051");
    CollisionQueryServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server Listening on " << server_address << std::endl;

    server->Wait();


}

int main(int argc, char *argv[]){

    RunServer();
    return 0;
}
