# This file is auto-generated from the current state of the database. Instead
# of editing this file, please use the migrations feature of Active Record to
# incrementally modify your database, and then regenerate this schema definition.
#
# This file is the source Rails uses to define your schema when running `bin/rails
# db:schema:load`. When creating a new database, `bin/rails db:schema:load` tends to
# be faster and is potentially less error prone than running all of your
# migrations from scratch. Old migrations may fail to apply correctly if those
# migrations use external dependencies or application code.
#
# It's strongly recommended that you check this file into your version control system.

ActiveRecord::Schema[7.1].define(version: 2024_08_06_032215) do
  create_table "carros", force: :cascade do |t|
    t.string "crv"
    t.string "nome"
    t.string "marca"
    t.integer "pessoa_id", null: false
    t.datetime "created_at", null: false
    t.datetime "updated_at", null: false
    t.index ["pessoa_id"], name: "index_carros_on_pessoa_id"
  end

  create_table "computadors", force: :cascade do |t|
    t.string "numserie"
    t.string "marca"
    t.integer "pessoa_id", null: false
    t.datetime "created_at", null: false
    t.datetime "updated_at", null: false
    t.index ["pessoa_id"], name: "index_computadors_on_pessoa_id"
  end

  create_table "dis_pes", force: :cascade do |t|
    t.integer "disciplina_id", null: false
    t.integer "pessoa_id", null: false
    t.datetime "created_at", null: false
    t.datetime "updated_at", null: false
    t.index ["disciplina_id"], name: "index_dis_pes_on_disciplina_id"
    t.index ["pessoa_id"], name: "index_dis_pes_on_pessoa_id"
  end

  create_table "disciplinas", force: :cascade do |t|
    t.string "codigo"
    t.string "nome"
    t.datetime "created_at", null: false
    t.datetime "updated_at", null: false
  end

  create_table "pessoas", force: :cascade do |t|
    t.string "rg"
    t.string "nome"
    t.integer "idade"
    t.string "cidade"
    t.datetime "created_at", null: false
    t.datetime "updated_at", null: false
  end

  add_foreign_key "carros", "pessoas"
  add_foreign_key "computadors", "pessoas"
  add_foreign_key "dis_pes", "disciplinas"
  add_foreign_key "dis_pes", "pessoas"
end
