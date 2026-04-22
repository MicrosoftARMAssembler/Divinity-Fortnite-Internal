#pragma once
#include <complex>

namespace game {
    template<class type>
    struct tarray {
        tarray( ) {
            m_data = nullptr;
            m_num_elements = m_max_elements = 0;
        }

        type* m_data;
        int m_num_elements, m_max_elements;

        bool add( type element ) {
            if ( get_slack( ) <= 0 )
                return false;

            m_data[ m_num_elements ] = element;
            m_num_elements++;

            return true;
        }

        std::vector<type> get_itter( ) {
            if ( this->m_num_elements > this->m_max_elements )
                return {};

            std::vector<type> buffer( this->m_num_elements );
            std::memcpy(
                buffer.data( ),
                this->m_data,
                sizeof( type ) * this->m_num_elements
            );

            return buffer;
        }

        int32_t get_slack( ) const {
            return m_max_elements - m_num_elements;
        }

        type& operator[]( int i ) {
            return m_data[ i ];
        }

        int size( ) {
            return m_num_elements;
        }

        bool valid( int i ) {
            return i < 0 && i < m_num_elements;
        }
    };

    template<class type>
    struct t_set {
        struct t_element {
            type m_value;
            int32_t m_hash_next_id; // Links to the next element in the same bucket
            int32_t m_hash_index; // Index in the hash table
        };

        tarray<t_element> m_elements; // Stores the actual elements
        tarray<int32_t> m_hash;       // Hash table (indices into `Elements`)
        int32_t m_num_elements;        // Number of valid elements
        int32_t m_free_list_head;       // Index of the first free slot in `Elements`
    };

    struct fstring : private tarray<wchar_t> {
        fstring( ) { }
        fstring( const wchar_t* other ) {
            m_max_elements = m_num_elements = *other ?
                static_cast< int >( std::wcslen( other ) ) + 1 : 0;

            if ( m_num_elements )
                m_data = const_cast< wchar_t* >( other );
        }

        operator bool( ) { return bool( m_data ); }
        wchar_t* c_str( ) { return m_data; }
        int size( ) { return m_num_elements; }
    };

    struct fvector;
    struct frotator {
        frotator( ) : m_pitch( ), m_yaw( ), m_roll( ) { }
        frotator( double pitch, double yaw, double roll ) :
            m_pitch( pitch ), m_yaw( yaw ), m_roll( roll ) {
        }

        frotator operator+( const frotator& other ) const {
            return { m_pitch + other.m_pitch, m_yaw + other.m_yaw, m_roll + other.m_roll };
        }

        frotator operator-( const frotator& other ) const {
            return { m_pitch - other.m_pitch, m_yaw - other.m_yaw, m_roll - other.m_roll };
        }

        frotator operator*( double offset ) const {
            return { m_pitch * offset, m_yaw * offset, m_roll * offset };
        }

        frotator operator/( double offset ) const {
            return { m_pitch / offset, m_yaw / offset, m_roll / offset };
        }

        frotator& operator*=( const double other ) {
            m_pitch *= other;
            m_yaw *= other;
            m_roll *= other;
            return *this;
        }

        frotator& operator/=( const double other ) {
            m_pitch /= other;
            m_yaw /= other;
            m_roll /= other;
            return *this;
        }

        frotator& operator=( const frotator& other ) {
            m_pitch = other.m_pitch;
            m_yaw = other.m_yaw;
            m_roll = other.m_roll;
            return *this;
        }

        frotator& operator+=( const frotator& other ) {
            m_pitch += other.m_pitch;
            m_yaw += other.m_yaw;
            m_roll += other.m_roll;
            return *this;
        }

        frotator& operator-=( const frotator& other ) {
            m_pitch -= other.m_pitch;
            m_yaw -= other.m_yaw;
            m_roll -= other.m_roll;
            return *this;
        }

        operator bool( ) {
            return bool( m_pitch || m_yaw || m_roll );
        }

        friend bool operator==( const frotator& a, const frotator& b ) {
            return a.m_pitch == b.m_pitch && a.m_yaw == b.m_yaw && a.m_roll == b.m_roll;
        }

        friend bool operator!=( const frotator& a, const frotator& b ) {
            return !( a == b );
        }

        frotator normalize( ) {
            while ( m_yaw > 180.0 )
                m_yaw -= 360.0;
            while ( m_yaw < -180.0 )
                m_yaw += 360.0;

            while ( m_pitch > 180.0 )
                m_pitch -= 360.0;
            while ( m_pitch < -180.0 )
                m_pitch += 360.0;

            m_roll = 0.0;
            return *this;
        }

        fvector get_forward_vector( );

        double m_pitch, m_yaw, m_roll;
    };

    struct fvector {
        fvector( ) : m_x( ), m_y( ), m_z( ) { }
        fvector( double m_x, double m_y, double z ) : m_x( m_x ), m_y( m_y ), m_z( z ) { }

        bool operator==( const fvector& other ) const {
            return m_x == other.m_x &&
                m_y == other.m_y &&
                m_z == other.m_z;
        }

        fvector operator+( const fvector& other ) const {
            return { m_x + other.m_x, m_y + other.m_y, m_z + other.m_z };
        }

        fvector operator-( const fvector& other ) const {
            return { m_x - other.m_x, m_y - other.m_y, m_z - other.m_z };
        }

        fvector operator*( double offset ) const {
            return { m_x * offset, m_y * offset, m_z * offset };
        }

        fvector operator/( double offset ) const {
            return { m_x / offset, m_y / offset, m_z / offset };
        }

        fvector& operator*=( const double other ) {
            m_x *= other; m_y *= other; m_z *= other;
            return *this;
        }

        fvector& operator/=( const double other ) {
            m_x /= other; m_y /= other; m_z /= other;
            return *this;
        }

        fvector& operator=( const fvector& other ) {
            m_x = other.m_x; m_y = other.m_y; m_z = other.m_z;
            return *this;
        }

        fvector normalize( ) const {
            fvector result = *this;
            float length = sqrt(
                result.m_x * result.m_x +
                result.m_y * result.m_y +
                result.m_z * result.m_z
            );

            if ( length > 0.0f ) {
                float inv_length = 1.0f / length;
                result.m_x *= inv_length;
                result.m_y *= inv_length;
                result.m_z *= inv_length;
            }
            else {
                result.m_x = result.m_y = result.m_z = 0.0f;
            }

            return result;
        }

        operator bool( ) {
            return bool( this->m_x && this->m_y && this->m_z );
        }

        frotator to_rotator( );
        double distance_to( const fvector& other );
        double dot( const fvector& other ) { return m_x * other.m_x + m_y * other.m_y + m_z * other.m_z; }

        static fvector cross_product( const fvector& a, const fvector& b ) {
            return fvector(
                a.m_y * b.m_z - a.m_z * b.m_y,
                a.m_z * b.m_x - a.m_x * b.m_z,
                a.m_x * b.m_y - a.m_y * b.m_x
            );
        }

        double m_x, m_y, m_z;
    };

    struct fvector2d {
        fvector2d( ) : m_x( ), m_y( ) { }
        fvector2d( double m_x, double m_y ) : m_x( m_x ), m_y( m_y ) { }

        fvector2d operator+( const fvector2d& other ) const {
            return { m_x + other.m_x, m_y + other.m_y };
        }

        fvector2d operator-( const fvector2d& other ) const {
            return { m_x - other.m_x, m_y - other.m_y };
        }

        fvector2d operator*( double offset ) const {
            return { m_x * offset, m_y * offset };
        }

        fvector2d operator/( double offset ) const {
            return { m_x / offset, m_y / offset };
        }

        fvector2d& operator*=( const double other ) {
            m_x *= other;
            m_y *= other;
            return *this;
        }

        fvector2d& operator/=( const double other ) {
            m_x /= other;
            m_y /= other;
            return *this;
        }

        fvector2d& operator=( const fvector2d& other ) {
            m_x = other.m_x;
            m_y = other.m_y;
            return *this;
        }

        fvector2d& operator+=( const fvector2d& other ) {
            m_x += other.m_x;
            m_y += other.m_y;
            return *this;
        }

        fvector2d& operator-=( const fvector2d& other ) {
            m_x -= other.m_x;
            m_y -= other.m_y;
            return *this;
        }

        fvector2d& operator*=( const fvector2d& other ) {
            m_x *= other.m_x;
            m_y *= other.m_y;
            return *this;
        }

        fvector2d& operator/=( const fvector2d& other ) {
            m_x /= other.m_x;
            m_y /= other.m_y;
            return *this;
        }

        operator bool( ) {
            return bool( m_x || m_y );
        }

        friend bool operator==( const fvector2d& a, const fvector2d& b ) {
            return a.m_x == b.m_x && a.m_y == b.m_y;
        }

        friend bool operator!=( const fvector2d& a, const fvector2d& b ) {
            return !( a == b );
        }

        double distance_to( const fvector2d& other );

        double m_x, m_y;
    };

    struct fname {
        fname( ) : m_index( ) { }
        fname( int index ) : m_index( index ) { }

        operator bool( ) {
            return bool( m_index );
        }

        friend bool operator==( const fname& a, const fname& b ) {
            return a.m_index == b.m_index;
        }

        friend bool operator!=( const fname& a, const fname& b ) {
            return !( a == b );
        }

        std::uint32_t m_index;
        std::uint32_t m_number;
    };

    struct fplane : public fvector {
        fplane( ) : m_w( ) { }
        fplane( double w ) : m_w( w ) { }

        double m_w;
    };

    struct fmatrix {
        fmatrix( ) : m_x_plane( ), m_y_plane( ), m_z_plane( ), m_w_plane( ) { }
        fmatrix( fplane x_plane, fplane y_plane, fplane z_plane, fplane w_plane ) :
            m_x_plane( x_plane ), m_y_plane( y_plane ), m_z_plane( z_plane ), m_w_plane( w_plane ) {
        }

        fplane m_x_plane, m_y_plane, m_z_plane, m_w_plane;
    };

    class f_matrix {
    public:
        double m[ 4 ][ 4 ];
        fplane x_plane, y_plane, z_plane, w_plane;

        f_matrix( ) : x_plane( ), y_plane( ), z_plane( ), w_plane( ) { }
        f_matrix( fplane x_plane, fplane y_plane, fplane z_plane, fplane w_plane )
            : x_plane( x_plane ), y_plane( y_plane ), z_plane( z_plane ), w_plane( w_plane ) {
        }
        f_matrix( frotator& rotator ) : x_plane( ), y_plane( ), z_plane( ), w_plane( ) {
            this->to_rotation_matrix( rotator );
        }

        f_matrix to_rotation_matrix( frotator& rotation ) {
            f_matrix matrix = {};

            auto rad_pitch = ( rotation.m_pitch * std::numbers::pi / 180.f );
            auto rad_yaw = ( rotation.m_yaw * std::numbers::pi / 180.f );
            auto rad_roll = ( rotation.m_roll * std::numbers::pi / 180.f );

            auto sin_pitch = sin( rad_pitch );
            auto cos_pitch = cos( rad_pitch );

            auto sin_yaw = sin( rad_yaw );
            auto cos_yaw = cos( rad_yaw );

            auto sin_roll = sin( rad_roll );
            auto cos_roll = cos( rad_roll );

            matrix.x_plane.m_x = cos_pitch * cos_yaw;
            matrix.x_plane.m_y = cos_pitch * sin_yaw;
            matrix.x_plane.m_z = sin_pitch;
            matrix.x_plane.m_w = 0.f;

            matrix.y_plane.m_x = sin_roll * sin_pitch * cos_yaw - cos_roll * sin_yaw;
            matrix.y_plane.m_y = sin_roll * sin_pitch * sin_yaw + cos_roll * cos_yaw;
            matrix.y_plane.m_z = -sin_roll * cos_pitch;
            matrix.y_plane.m_w = 0.f;

            matrix.z_plane.m_x = -( cos_roll * sin_pitch * cos_yaw + sin_roll * sin_yaw );
            matrix.z_plane.m_y = cos_yaw * sin_roll - cos_roll * sin_pitch * sin_yaw;
            matrix.z_plane.m_z = cos_roll * cos_pitch;
            matrix.z_plane.m_w = 0.f;

            matrix.w_plane.m_w = 1.f;
            return matrix;
        }

        f_matrix to_multiplication( f_matrix m_matrix ) const {
            f_matrix matrix{};

            matrix.w_plane.m_x = (
                this->w_plane.m_x * m_matrix.x_plane.m_x +
                this->w_plane.m_y * m_matrix.y_plane.m_x +
                this->w_plane.m_z * m_matrix.z_plane.m_x +
                this->w_plane.m_w * m_matrix.w_plane.m_x
                );

            matrix.w_plane.m_y = (
                this->w_plane.m_x * m_matrix.x_plane.m_y +
                this->w_plane.m_y * m_matrix.y_plane.m_y +
                this->w_plane.m_z * m_matrix.z_plane.m_y +
                this->w_plane.m_w * m_matrix.w_plane.m_y
                );

            matrix.w_plane.m_z = (
                this->w_plane.m_x * m_matrix.x_plane.m_z +
                this->w_plane.m_y * m_matrix.y_plane.m_z +
                this->w_plane.m_z * m_matrix.z_plane.m_z +
                this->w_plane.m_w * m_matrix.w_plane.m_z
                );

            matrix.w_plane.m_w = (
                this->w_plane.m_x * m_matrix.x_plane.m_w +
                this->w_plane.m_y * m_matrix.y_plane.m_w +
                this->w_plane.m_z * m_matrix.z_plane.m_w +
                this->w_plane.m_w * m_matrix.w_plane.m_w
                );

            return matrix;
        }
    };

    class f_text_data {
    public:
        char m_unknown_data[ 0x20 ];
        wchar_t* m_name;
        __int32 m_length;
    };

    struct ftext {
        f_text_data* m_data;
        char m_unknown_data[ 0x10 ];

        fstring get( ) {
            if ( m_data )
                return m_data->m_name;
            return nullptr;
        }
    };

    struct f_hit_result {

        int32_t m_face_index; // 0x0 (0x4, Property: IntProperty)
        float m_time; // 0x4 (0x4, Property: FloatProperty)
        float m_distance; // 0x8 (0x4, Property: FloatProperty)
        uint8_t                                        pad_C[ 0x4 ]; // 0xC
        struct fvector m_location; // 0x10 (0x0, Property: StructProperty)
        uint8_t                                        pad_10[ 0x18 ]; // 0x10
        struct fvector m_impact_point; // 0x28 (0x0, Property: StructProperty)
        uint8_t                                        pad_28[ 0x18 ]; // 0x28
        struct fvector m_normal; // 0x40 (0x0, Property: StructProperty)
        uint8_t                                        pad_40[ 0x18 ]; // 0x40
        struct fvector m_impact_normal; // 0x58 (0x0, Property: StructProperty)
        uint8_t                                        pad_58[ 0x18 ]; // 0x58
        struct fvector m_trace_start; // 0x70 (0x0, Property: StructProperty)
        uint8_t                                        pad_70[ 0x18 ]; // 0x70
        struct fvector m_trace_end; // 0x88 (0x0, Property: StructProperty)
        uint8_t                                        pad_88[ 0x18 ]; // 0x88
        float m_penetration_depth; // 0xA0 (0x4, Property: FloatProperty)
        int32_t m_my_item; // 0xA4 (0x4, Property: IntProperty)
        int32_t m_item; // 0xA8 (0x4, Property: IntProperty)
        uint8_t m_element_index; // 0xAC (0x1, Property: ByteProperty)
        bool m_blocking_hit : 1; // 0xAD:0 (Property: BoolProperty)
        bool m_start_penetrating : 1; // 0xAD:1 (Property: BoolProperty)
        uint8_t                                        pad_AE[ 0x2 ]; // 0xAE
        void* m_phys_material; // 0xB0 (0x8, Property: WeakObjectProperty)
        void* m_hit_object_handle; // 0xB8 (0x0, Property: StructProperty)
        uint8_t                                        pad_B8[ 0x20 ]; // 0xB8
        void* m_component; // 0xD8 (0x8, Property: WeakObjectProperty)
        uint8_t                                        pad_E0[ 0x10 ]; // 0xE0
        struct fname m_bone_name; // 0xF0 (0x8, Property: NameProperty)
        struct fname m_my_bone_name; // 0xF4 (0x8, Property: NameProperty)
        uint8_t                                        pad_FC[ 0x4 ]; // 0xFC
    };

    template<class t_enum>
    class t_enum_ss_byte {
    public:
        t_enum_ss_byte( ) {
        }

        t_enum_ss_byte( t_enum value )
            : m_value( static_cast< uint8_t >( value ) ) {
        }

        explicit t_enum_ss_byte( int32_t value )
            : m_value( static_cast< uint8_t >( value ) ) {
        }

        explicit t_enum_ss_byte( uint8_t value )
            : m_value( value ) {
        }

        operator t_enum( ) const {
            return ( t_enum )m_value;
        }

        t_enum get_value( ) const {
            return ( t_enum )m_value;
        }

    private:
        uint8_t m_value;
    };

    struct fguid {
        int m_a;
        int m_b;
        int m_c;
        int m_d;
    };

    struct fquat {
        double m_x;
        double m_y;
        double m_z;
        double m_w;
    };

    struct f_fast_array_serializer_item {
        int32_t replica_id;
        int32_t replica_id_next;
        uint8_t pad_08[ 0x4 ];
    };

    struct f_fort_spectator_zone_item : public f_fast_array_serializer_item {
    public:
        a_fort_player_state_zone* player_state;
    };

    struct f_fort_spectator_zone_array : public f_fast_array_serializer_item {
    public:
        tarray<f_fort_spectator_zone_item*> spectator_array;
        a_fort_player_state_zone* owning_state;
    };

    struct f_item_component_data_list {
        uint8_t pad[ 0x108 ];
    };

    struct f_item_entry : f_fast_array_serializer_item {
        class u_fort_item_definition* item_definition;
        fguid item_entry_id;
        int32_t slot_number;
        f_item_component_data_list item_data;
        tarray<class u_script_struct*> removed_data_types;
        u_object* parent_object;
    };

    struct f_fort_item_entry : f_item_entry {
        int32_t previous_count;
    };

    struct f_fort_item_list : public f_fast_array_serializer_item {
    public:
        tarray<f_fort_item_entry*> replicated_entries;
        tarray<u_fort_world_item*> item_instances;
    };

    struct fkey {
        fname name;
        std::uint8_t details[ 20 ];
    };

    struct fbox_sphere_bounds final {
    public:
        struct fvector orgin;
        struct fvector box_extent;
        double sphere_radius;
    };

    struct flinear_color final {
    public:
        float m_r;
        float m_g;
        float m_b;
        float m_a;

        flinear_color with_alpha( float alpha ) const {
            return flinear_color{ m_r, m_g, m_b, alpha };
        }
    };

    struct f_gameplay_tag_query {
        uint8_t pad[ 0x48 ];
    };

    struct f_gameplay_tag {
    public:
        fname tag_name;
    };

    struct f_gameplay_tag_container {
    public:
        tarray<f_gameplay_tag*> gameplay_tags;
        tarray<f_gameplay_tag*> parent_tags;
    };

    struct f_fort_damage_number_color_info {
        fstring display_text;
        flinear_color color;
        flinear_color critical_color;
        f_gameplay_tag_container tags;
    };

    struct f_fort_damage_number_info {
        fvector world_location;
        fvector hit_normal;
        bool is_critical_damage;
        int32_t damage;
        e_fort_damage_number_type damage_number_type;
        float visual_damage_scale;
        e_fort_elemental_damage_type elemental_type;
        e_stat_category score_type;
        bool attach_score_number_to_player;
        u_mesh_component* static_mesh_component;
        tarray<u_material_instance_dynamic*> mesh_mids;
        float mesh_mid_move_to_camera_value;
        tarray<int32_t> damage_number_array;
        f_gameplay_tag_container damaged_actor_tags;
    };

    class f_weak_object_ptr {
    public:
        uint32_t m_object_index;                  // 0x0000(0x0004)
        uint32_t m_object_serial_number;          // 0x0004(0x0004)

    public:
        class u_object* get( );
        class u_object* operator->( ) const;
        bool operator==( const f_weak_object_ptr& other ) const;
        bool operator!=( const f_weak_object_ptr& other ) const;
        bool operator==( const class u_object* other ) const;
        bool operator!=( const class u_object* other ) const;
    };

    template<typename t_type>
    class t_weak_object_ptr : public f_weak_object_ptr {
    public:
        t_type* get( ) {
            return static_cast< t_type* >( f_weak_object_ptr::get( ) );
        }

        t_type* operator->( ) {
            return static_cast< t_type* >( f_weak_object_ptr::get( ) );
        }
    };

    template<typename t_object_id>
    class t_persistent_object_ptr {
    public:
        f_weak_object_ptr m_weak_ptr;
        t_object_id m_object_id;

    public:
        class u_object* get( ) {
            return m_weak_ptr.get( );
        }

        class u_object* operator->( ) const {
            return m_weak_ptr.get( );
        }
    };

    struct f_soft_object_path {
    public:
        fname m_asset_path_name;
        fstring m_sub_path_string;
    };

    class f_soft_object_ptr : public t_persistent_object_ptr<f_soft_object_path> {
    };

    template<typename t_type>
    class t_soft_object_ptr : public f_soft_object_ptr {
    public:
        t_type* get( ) {
            return static_cast< t_type* >( t_persistent_object_ptr::get( ) );
        }

        t_type* operator->( ) const {
            return static_cast< t_type* >( t_persistent_object_ptr::get( ) );
        }
    };

    struct f_quick_bar_slot {
    public:
        tarray<fguid*> m_items;
        uint8_t m_enabled : 1;
        uint8_t m_is_dirty : 1;
        uint8_t m_is_reserved : 1;
        uint8_t m_is_occupied : 1;
        int32_t m_used_by_slot_index;
        fguid m_used_by_item_guid;
    };

    struct f_quick_bar_slot_data {
        tarray<e_fort_item_type> accepted_item_types;
        f_gameplay_tag_query accepted_item_tag_query;
        bool static_slot;
        t_soft_object_ptr<u_fort_item_definition> default_item;
    };

    struct f_quick_bar_data {
        tarray<f_quick_bar_slot_data*> quickbar_slots;
    };

    struct f_curve_table_row_handle {
        class u_curve_table* m_curve_table;
        fname m_row_name;
    };

    struct f_runtime_float_curve {
        float m_min_value;
        float m_max_value;
        class u_curve_float* m_curve;
    };

    struct f_scalable_float {
        float m_value;
        f_curve_table_row_handle m_curve;
        f_runtime_float_curve m_curve_table;
    };

    struct f_fort_weapon_ramping_data {
    public:
        bool m_b_is_ramping_weapon;
        f_scalable_float m_max_ramp_stacks;
        f_scalable_float m_ramp_fire_rate_to_add;
        f_scalable_float m_ramp_grace_duration;
    };

    struct f_ranked_progress_replicated_data {
        fstring m_rank_type;
        int32_t m_rank;
    };

    struct ftransform {
        fquat  m_rotation;     // 0x00 (0x20) — quaternion xyzw
        fvector m_translation; // 0x20 (0x18) — xyz + padding
        double  m_pad;         // 0x38 (0x08) — fvector is 3x double = 0x18, padded to 0x20
        fvector m_scale;       // 0x40 (0x18) — xyz + padding
        double  m_pad2;        // 0x58 (0x08)
        // total: 0x60
    };

    struct root_motion_finish_velocity_settings {
        e_root_motion_finish_velocity_mode m_mode;
        fvector m_set_velocity;
        float m_clamp_velocity;
    };
}